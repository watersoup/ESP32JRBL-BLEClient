#include <Arduino.h>
#include <ArduinoJson.h>
#include <bleClientObj.h>
#include <BLEDevice.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h> 
#include <ElegantOTA.h>
#include <LittleFSsupport.h>
#include "wifiData.h"

#define DEBUG 1
#define  SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 2000
#define  MIN_PRESS_TIME 200
#define SPIFFS LittleFS 

AsyncWebServer server(80);
DNSServer dnsServer;
AsyncWebSocket ws("/ws");

AsyncWiFiManager *wifiManager = new AsyncWiFiManager(&server, &dnsServer);

unsigned long ota_progress_millis = 0;
int BUILTINPIN = 8;
const int onOffButton = 20;
const int intensityButton = 21;
int currentState;
long int pressedTime, pressDuration;
unsigned long advertisingStartTime = 0;
const unsigned long advertisingTimeout = 180000; // 180 seconds
bool intensityButtonPressed = false;

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
void notifyError(String message){
  ws.textAll("LOG : "+message);
}
void notifyError(AsyncWebSocketClient *client, String  message){
  client->text("LOG : "+message);
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
// / Function to handle intensity button press
void handleIntensityButtonPress() {
    
    // Check for intensity button press

    if (digitalRead(intensityButton) == HIGH) {
        pressedTime = millis();
        // attachAll();
        // Button pressed, handle the press
        // slowOpen();
        Serial.println("Intensity Button pressed....slow opening/closing");

        delay(30);
        // setup button to start blueTooth
        if(digitalRead(onOffButton)== HIGH){
          Serial.println("Bluetooth Scann to be started ...");
          if (! bleInst->isConnected()){
            bleInst->doScan=true;
          } else {
            // start wifi service and turn off after 5 mins;
            Serial.println(" starting WIFI server");
            
          }
        }
        intensityButtonPressed = true; 
    } else if (intensityButtonPressed) {
        Serial.println("Intensity button released...handle the flags");
        // Button released, handle the release
        // if (myservo.attached(sPin[k]) != 253 ){
        //         Serial.printf("Pin[%d] is attached",k);
        // }
        intensityButtonPressed = false;
        // if (myservo.read(sPin[0])> SERVO_MIN_ANGLE) {
        //     blindsOpen = true;
        // }
        // detachAll();
    }
}

// Function to handle open button press
void handleOnOffButtonPress(){

    currentState = digitalRead(onOffButton);
    if (currentState == HIGH){
        Serial.println(" handleOnoffButtonPress..");
        pressedTime = millis();

        // setup button to start blueTooth
        if(digitalRead(intensityButton)== HIGH){
          Serial.println("Bluetooth Scann to be started ...");
          if (! bleInst->isConnected()){
            bleInst->doScan=true;
          } else {
            // start wifi service and turn off after 5 mins;
            Serial.println(" starting WIFI server");
          }
        }

        // Button pressed
        // Wait for button release
        while (digitalRead(onOffButton) == HIGH) {
            delay(10);
        }
        pressDuration = millis() - pressedTime;
        if (pressDuration >= SHORT_PRESS_TIME){
            //set the high level based on the current angle;
            // if (blindsOpen){
                Serial.println(" *****Setting the max angles");
                // setOpeningAngle();
            // }
        } else {

            delay(50);
            // openOrCloseBlind();
            // delay(200);

        }
    }
}


void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    // New WebSocket client connected
    if (DEBUG) Serial.println("WebSocket client connected");
    client->text("LOG : WebSocket client connected !!! NEW " + String(ws.count()));
  } else if (type == WS_EVT_DISCONNECT) {
    // WebSocket client disconnected
    if (DEBUG) Serial.println("WebSocket client disconnected");
    notifyError(client,"WebSocket client disconnected :" + String(ws.count()));
  }else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    // try{
      if (info->opcode == WS_TEXT) {
        // Handle text data received from the client
        String message = String( (const char *)data).substring(0, len);
        // Serial.println("Received message from WebSocket client: " + message);
        
        // Route for handling the momentary setup button click
        if ( message.indexOf("setupLimit") >= 0){      
            Serial.println(" REceived: setupLimit");
        }
        // route the click of Close/Open blinds.
        if (message.indexOf("status") >=0 ){
            Serial.println(" HandleOnOff");
        }
        // Route for setting the slider value
        if (message.indexOf("sliderValue")>=0){
          Serial.println(" REceived: sliderValue");
        }
        // Route for the very first request after load;
        if (message.indexOf("initialize")>=0){
          // notify client of existing parameter if they have already
          // been initialized;
          Serial.println(" Received: Initialize");
        }
      }
  }
}

void handleFirstLoad( AsyncWebServerRequest *request){
  Serial.println( "FirstLoad request from : " + request->client()->localIP());
}

void serverSetup() {
  // Serial.println("Server Setup in progress...");
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);

  // Route for serving the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  // Route for handling the first load
  server.on("/firstLoad", HTTP_POST, [](AsyncWebServerRequest *request) {
    handleFirstLoad(request);
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
  
    if (DEBUG) Serial.println(" *** factoryReset.."+ request->method());
    if ( request->hasArg("resetKey")){
      String key = request->arg("resetKey");
      if (key == "XYZ123"){
        // C1.FactoryReset();

        // Respond with a success message
        ws.textAll("{\"factoryResetStatus\": \"success\"}");
        if (DEBUG) Serial.println("Factory reset completed successfully");
      }
    }
  });
}

void initiate_buttons(){
    pinMode(onOffButton, INPUT_PULLDOWN);
    pinMode(intensityButton, INPUT_PULLDOWN);
    digitalWrite(onOffButton, LOW);
    digitalWrite(intensityButton, LOW);
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
    pinMode(BUILTINPIN,OUTPUT);
    
    blink(BUILTINPIN, 1, 1000);
    bleInst = new bleClientObj();
    Serial.begin(115200);

    // start WIFI;    
    startWifi();

    //initialize filesystem
    initFS();
    blink(BUILTINPIN,2,200);

    server.serveStatic("/", LittleFS, "/");  
    blink(BUILTINPIN,2,1000);
    // server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    //     request->send(200,(const char *)"text/html", (const char *)indexpage, nullptr);
    // });
    // setup the server
    serverSetup();
    blink(BUILTINPIN,4,200);

    setOTA();
    blink(BUILTINPIN,2,1000);

    
    server.begin();
    blink(BUILTINPIN,5,200);

    Serial.println("Starting Arduino BLE Client application...");
    bleInst->scan();
    blink(BUILTINPIN,2,1000);

    initiate_buttons();
    blink(BUILTINPIN,6,200);
}

void loop()
{
    // Check for open button press
    handleOnOffButtonPress();

    // check if intensity button pressed
    handleIntensityButtonPress();

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
        blink(BUILTINPIN, 1, 500);
    }
    else if(bleInst->doScan)
    {
        blink(BUILTINPIN,2, 500);
        Serial.println( " Scanning ... again...");
          // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
        bleInst->scan();
    }
    delay(500); 
}