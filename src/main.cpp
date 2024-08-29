#include <Arduino.h>
#include <ArduinoJson.h>
#include <bleClientObj.h>
#include <BLEDevice.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h> 
#include <ElegantOTA.h>
#include "wifiData.h"



AsyncWebServer server(80);
DNSServer dnsServer;
AsyncWebSocket ws("/ws");

AsyncWiFiManager *wifiManager = new AsyncWiFiManager(&server, &dnsServer);

unsigned long ota_progress_millis = 0;
int BUILTINPIN = 8;
int count =0;
bleClientObj *bleInst=nullptr;
std::string g_rxValue;
String g_txValue;

void blink(const int PIN, int times=1, long int freq=500){
  for(int i = 0; i < times; i++){
    digitalWrite(PIN, LOW);
    delay(freq);
    digitalWrite(PIN, HIGH);
    delay(freq);
  }
}

void onOTAStart() {
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final) {
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("OTA update finished successfully!");
  } else {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}
void setOTA(){
  // set up OTA
  ElegantOTA.begin(&server);
  ElegantOTA.setAuth("Admin", "admin");
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
}
void startWifi(){

  WiFi.begin(SSID, PASS_WD);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
    Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
void setup()
{
    // start WIFI;
    blink(BUILTINPIN, 1, 1000);
    bleInst = new bleClientObj();
    Serial.begin(115200);
    pinMode(BUILTINPIN,OUTPUT);
    blink(BUILTINPIN,2,200);
    startWifi();
    blink(BUILTINPIN,2,1000);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(200,(const char *)"text/html", (const char *)indexpage, nullptr);
    });

    blink(BUILTINPIN,4,200);
    setOTA();
    blink(BUILTINPIN,2,1000);
    blink(BUILTINPIN,5,200);
    server.begin();
    blink(BUILTINPIN,2,1000);

    Serial.println("Starting Arduino BLE Client application...");
    bleInst->scan();
    blink(BUILTINPIN,10,200);
}


void loop()
{

  /* If the flag "doConnect" is true, then we have scanned for and found the desired
    BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
    connected we set the connected flag to be true. */
  
    if (bleInst->doConnect == true)
    {
        if (bleInst->connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
            bleInst->doScan=false;
            // if connected shutdown the wifi
        } 
        else
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
        bleInst->doConnect = false;
    }

    /* If we are connected to a peer BLE Server, update the characteristic each time we are reached
      with the current time since boot */
    if (bleInst->connected)
    {
        // g_rxValue = pRemoteChar_2->readValue();
        // Serial.println("read:" + String(g_rxValue.c_str()));
        
        /* Set the characteristic's value to be the array of bytes that is actually a string */
        int rand_num = -random(1000);
        g_txValue = "Cl-2:" + String(rand_num);
        Serial.println("---> " + String(rand_num));
        bleInst->writeStatus(g_txValue);
        delay(1000);
 
    }
    else if(bleInst->doScan)
    {
        blink(BUILTINPIN,2, 500);
        Serial.println( " Scanning ... again...");
          // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
        bleInst->scan();
    }
    delay(1000); 
}