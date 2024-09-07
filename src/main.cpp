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
#include "motorObj.h"

#define DEBUG 1
#define  SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 2000
#define  MIN_PRESS_TIME 200
#define SPIFFS LittleFS 
#define CLOSEBLINDS 0
#define OPENBLINDS 1

AsyncWebServer server(80);
DNSServer dnsServer;
AsyncWebSocket ws("/ws");

AsyncWiFiManager *wifiManager = new AsyncWiFiManager(&server, &dnsServer);

unsigned long ota_progress_millis = 0;
unsigned long wifiStartMillis = 0;
unsigned long bleStartMillis = 0;
unsigned long lastUpdateTime;
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

// create a motor object with 1 Servo;
motorObj *mymotor;
int directions[] = {1,1,1};


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

bool turnOnWiFi(){
  Serial.println("turning Wifi On ...");
  WiFi.mode(WIFI_STA);              // Set Wi-Fi mode to Station (client)
  WiFi.begin();                     // Begin reconnecting to the last saved AP
  
  // Wait until Wi-Fi is connected or timeout after 10 seconds
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWi-Fi connected!");
      Serial.println(WiFi.localIP());  // Print the local IP
  } else {
      Serial.println("\nFailed to reconnect to Wi-Fi.");
      return false;
  }
  // start the timer
  wifiStartMillis = millis();
  return true;
}
bool turnOffWiFi(){
  Serial.println("turning Wifi off....");
  // Disconnect Wi-Fi and do not remove saved credentials
  // Turn off the Wi-Fi radio
  if (WiFi.disconnect(false) && WiFi.mode(WIFI_OFF)) {
    return true;
  }
  return false;
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
        if(!intensityButtonPressed) mymotor->attachAll();
        // Button pressed, handle the press
        mymotor->slowOpen();
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
            if ( ! turnOnWiFi() ){
              blink(BUILTINPIN, 5, 100);
            }
          }
        }
        intensityButtonPressed = true; 
    } else if (intensityButtonPressed) {
        Serial.println("Intensity button released...handle the flags");
        // Button released, handle the release
        intensityButtonPressed = false;
        mymotor->cleanUpAfterSlowOpen();
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
            bleStartMillis = millis();
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
            if (mymotor->isBlindOpen()){
                Serial.println(" *****Setting the max angles");
                mymotor->setOpeningAngle();
            }
        } else {
            
            mymotor->openOrCloseBlind();
        }
    }
}

void notifyClients( int x = 1){
  JsonDocument  jsonDoc;
  jsonDoc["blindName"] = mymotor->getBlindName();
  int openflag =mymotor->isBlindOpen()?1:0;
  int limitflag = mymotor->getLimitFlag();
  int pos = mymotor->getPositionOfSlider();
  if ( x%2 == 0 ){
   jsonDoc["status"] =openflag;
  }
  if (x%3 == 0){
   jsonDoc["limitSetupFlag"] =limitflag;
  }
  if (x%5 == 0){
   jsonDoc["sliderPosition"] = pos;
  }

  jsonDoc.shrinkToFit();
  String jsonString;
  serializeJson(jsonDoc,jsonString);
  if (DEBUG) Serial.println(" NotifyClient " + String(x)+ " :" + jsonString);
  ws.textAll(jsonString);
  lastUpdateTime = millis();
  delay(5*x);
  
}
void notifyLog( String message){
  ws.textAll("LOG :"+ message);
}

void handleSliderUpdate(String message) { //GOOD
  // Handle setting the open range value based on the received value
  JsonDocument  jsonDoc;
  deserializeJson(jsonDoc, message);
  jsonDoc.shrinkToFit();
  int openRangeValue  = jsonDoc["sliderValue"].as<int>();  

  mymotor->moveBlinds(mymotor->getPositionOfMotor(openRangeValue));
  if ( DEBUG) Serial.println("WS_setSlider="+ mymotor->status + "ing " + String((const char*)jsonDoc["sliderValue"]));
  notifyClients(10);
}


void handleSetupClick(AsyncWebSocketClient *client, String message){
  // Parse the JSON data
  JsonDocument  jsonDoc;
  deserializeJson(jsonDoc, message);
  jsonDoc.shrinkToFit();
  // Handle the data as needed
  int buttonStatus = jsonDoc["setupLimit"].as<int>();
  int currpos;
  if(DEBUG) Serial.println( "handleSetupClick " + String(buttonStatus));

  long int slidervalue = -1;
  if (mymotor->getLimitFlag() < 3){
  
  currpos = mymotor->ifRunningHalt();

  // opening status then set UpperMost position
    if (buttonStatus && (mymotor->getLimitFlag() != 2) ){
      if (!mymotor->isBlindOpen())  client->text("LOG : Blind is closed, it should be open !!!");
      slidervalue = mymotor->setWindowMax(currpos);
      // if (DEBUG) Serial.println("SetupClick: max  Slider Position:"+ String(mymotor->getPositionOfSlider()));
      //* 0 on the other end means completely open, need to be readjusted;

    }else if ((buttonStatus==0) && (mymotor->getLimitFlag() != 1)){
      if (mymotor->isBlindOpen())  client->text("LOG : Blind is Open, it should be closed !!!");
      slidervalue = mymotor->setWindowLow(currpos);
      // if (DEBUG) Serial.println("SetupClick : Low  Slider Position:"+ String(mymotor->getPositionOfSlider()));
      // if blind is no closed notify of error in the co-ordination;
    }
    notifyClients(30);

    if (DEBUG)
      Serial.println("ws/setupLimit: sliderPos:"+ String(slidervalue) + " Status:"+
          String(mymotor->isBlindOpen()) + " Limit:" + String(mymotor->getLimitFlag()));
  }
}

void handleOnOff(String message){
  // Parse the JSON data
  JsonDocument  jsonDoc;
  deserializeJson(jsonDoc, message);
  jsonDoc.shrinkToFit();
  // Handle the data as needed
  int blindsStatus = jsonDoc["status"].as<int>();
  if(DEBUG){
    Serial.print(" handleOnOff .." + String(mymotor->isBlindOpen()));
    Serial.flush();
  } 
  if (blindsStatus == OPENBLINDS ) {
      // call appropriate function to open the blind
    if(DEBUG) Serial.println(" Opening .." );
    mymotor->openBlinds();
  } else if ( blindsStatus == CLOSEBLINDS){
    // call function to close blinds
    if(DEBUG) Serial.println(" closing ..");
    mymotor->closeBlinds();
  }
  notifyClients(2);
}

// handling factory reset request with secretkey;
// should be called as <ipAddress>/reset?resetKey=<secretkey>
void handleFactoryReset(String message) {
    // Parse the JSON data
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, message);

    // Check if the factoryReset flag is true and the provided secret key is correct
    if (jsonDoc.containsKey("reset") ) {
        
        // Get the secret key from the message
        String secretKey = jsonDoc["reset"].as<String>();
        
        // Perform additional checks if needed, and then proceed with the factory reset logic
        if (secretKey == "XYZ123"){
          // For example, you can clear the blindName
          mymotor->FactoryReset();
          ESP.restart();
          
        }
        // Respond with a success message
        ws.textAll("{\"factoryResetStatus\": \"success\"}");

        if (DEBUG) Serial.println("Factory reset completed successfully");
    } else {
        // Respond with an error message
        ws.textAll("{\"factoryResetStatus\": \"error\", \"message\": \"Invalid factory reset request.\"}");
        if (DEBUG) Serial.println("Invalid factory reset request");
    }
}

void handleFirstLoad(AsyncWebServerRequest *request){
   String JSONstring = "";
  bool reconnectFlag = false;
  if (request->hasArg("blindName") && mymotor->getBlindName() == ""){
    mymotor->setBlindName(request->arg("blindName"));
    reconnectFlag=true;

    if (request->hasArg("rightSide") ){
      String direction = request->arg("rightSide");
      direction.toUpperCase();
      int dir = (direction.compareTo("YES")==0? 1 : -1);
      mymotor->setSide(dir);
      if (DEBUG ) Serial.println("Direction: " + String(dir) + " "+ direction );
    }
    int blindsOpenFlag = (mymotor->isBlindOpen()?OPENBLINDS:CLOSEBLINDS);

    JSONstring = "{\"limitSetupFlag\":"+ String( mymotor->getLimitFlag()) + 
                      ",\"blindName\":\"" + mymotor->getBlindName() +
                      "\",\"status\":" + String(blindsOpenFlag) + "}";

  }
  if (DEBUG) Serial.println( JSONstring);
  request->send(200, "application/json", JSONstring );  
  if (reconnectFlag){
  //   // rename the wifi site server to the name of the blindname
    WiFi.hostname(mymotor->getBlindName());
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
            handleSetupClick(client, message);
        }
        // route the click of Close/Open blinds.
        if (message.indexOf("status") >=0 ){
            Serial.println(" HandleOnOff");
            handleOnOff(message);
        }
        // Route for setting the slider value
        if (message.indexOf("sliderValue")>=0){
          Serial.println(" REceived: sliderValue");
          handleSliderUpdate(message);
        }
        // Route for the very first request after load;
        if (message.indexOf("initialize")>=0){
          // notify client of existing parameter if they have already
          // been initialized;
          Serial.println(" Received: Initialize");
          notifyClients(30);
        }
      }
  }
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
        // mymotor->FactoryReset();

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

bool isWifiOn(){

  if (WiFi.getMode() != WIFI_OFF){
    return true;
  }
  return false;
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
  wifiStartMillis = millis();
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

    // setup the server
    serverSetup();
    blink(BUILTINPIN,4,200);

    setOTA();
    blink(BUILTINPIN,2,1000);

    // create motor instance;
    mymotor = new motorObj(1);

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
        bleInst->doConnect = false;
    }else if(bleInst->doScan)
    {
        blink(BUILTINPIN,2, 500);
        Serial.println( " Scanning ... again...");
        bleInst->scan();
        delay(1000);
        if ( ( millis() - bleStartMillis ) > 120000 ) {
          bleInst->doScan=false;
        }
    }
    if (isWifiOn() && (millis() - wifiStartMillis) > 120000 ){
      if ( turnOffWiFi()) {
        Serial.println("Wifi Turned off...");
        blink(BUILTINPIN, 5, 100);
      }
    }
    delay(500); 
}