#include <Arduino.h>
#include <ArduinoJson.h>
#include <bleClientObj.h>
#include <BLEDevice.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <esp_mac.h>
#include <ESPAsyncWiFiManager.h>
#include <ElegantOTA.h>
#include <LittleFSsupport.h>
#include "wifiData.h"
#include "motorObj.h"

#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG 1
#define SHORT_PRESS_TIME 500
#define LONG_PRESS_TIME 2000
#define MIN_PRESS_TIME 200
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
bool isInitialized = false;


int count = 0;
bleClientObj *bleInst = nullptr;
std::string g_rxValue;
String g_txValue;

// create a motor object with 1 Servo;
motorObj *mymotor;
int directions[] = {1, 1, 1,1};

void blink(const int PIN, int times = 1, long int freq = 500)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(PIN, LOW);
    delay(freq);
    digitalWrite(PIN, HIGH);
    delay(freq);
  }
}
void notifyError(String message)
{
  ws.textAll("LOG : " + message);
}
void notifyError(AsyncWebSocketClient *client, String message)
{
  client->text("LOG : " + message);
}

bool isWifiOn()
{

  if (WiFi.getMode() != WIFI_OFF)
  {
    return true;
  }
  return false;
}

void startWifi()
{

  WiFi.begin(SSID, PASS_WD);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
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

bool turnOnWiFi()
{
  Serial.println("turning Wifi On ...");
  WiFi.mode(WIFI_STA); // Set Wi-Fi mode to Station (client)
  WiFi.begin();        // Begin reconnecting to the last saved AP

  // Wait until Wi-Fi is connected or timeout after 10 seconds
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
  {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWi-Fi connected!");
    Serial.println(WiFi.localIP()); // Print the local IP
  }
  else
  {
    Serial.println("\nFailed to reconnect to Wi-Fi.");
    return false;
  }
  // start the timer
  wifiStartMillis = millis();
  return true;
}

bool turnOffWiFi()
{
  Serial.println("turning Wifi off....");
  // Disconnect Wi-Fi and do not remove saved credentials
  // Turn off the Wi-Fi radio
  if (WiFi.disconnect(false) && WiFi.mode(WIFI_OFF))
  {
    return true;
  }
  return false;
}
void onOTAStart()
{
  // Log when OTA has started
  Serial.println("OTA update started!");
  // <Add your own code here>
}

void onOTAProgress(size_t current, size_t final)
{
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000)
  {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

void notifyServer(int x = 1)
{
  JsonDocument jsonDoc;
  String jsonString ;

  jsonDoc["blindName"] = mymotor->getBlindName();
  int openflag = mymotor->isBlindOpen() ? 1 : 0;
  int limitflag = mymotor->getLimitFlag();
  int pos = mymotor->getPositionOfSlider();
  if (x % 2 == 0)
  {
    jsonDoc["status"] = openflag;
  }
  if (x % 3 == 0)
  {
    jsonDoc["limitSetupFlag"] = limitflag;
  }
  if (x % 5 == 0)
  {
    jsonDoc["sliderPosition"] = pos;
  }

  jsonDoc.shrinkToFit();
  serializeJson(jsonDoc, jsonString);
  if (DEBUG)
    Serial.println(" NotifyClient " + String(x) + " :" + jsonString);

  ws.textAll(jsonString);
  // if BLE server active then notify through BLE object as well
  if (bleInst != nullptr && bleInst->isConnected())
  {
    bleInst->notifyBLEServer(x);
  }
  lastUpdateTime = millis();
  delay(5 * x);
}

// / Function to handle intensity button press
void handleIntensityButtonPress()
{
  // Check for intensity button press
  if (digitalRead(intensityButton) == HIGH)
  {
    pressedTime = millis();
    if (!intensityButtonPressed)
      mymotor->attachAll();
    // Button pressed, handle the press
    mymotor->slowOpen();
    Serial.println("Intensity Button pressed....slow opening/closing");
    delay(30);
    intensityButtonPressed = true;

    // setup button to start blueTooth
    if (digitalRead(onOffButton) == HIGH)
    {
      Serial.println("Bluetooth Scann to be started ...");
      if (!bleInst->isConnected())
      {
        bleInst->doScan = true;
      }
      else
      {
        // start wifi service and turn off after 5 mins;
        Serial.println(" starting WIFI server");
        if (!turnOnWiFi())
        {
          blink(BUILTINPIN, 5, 100);
        }
      }
      intensityButtonPressed = false;
    }
  }
  else if (intensityButtonPressed)
  {
    Serial.println("Intensity button released...handle the flags");
    // Button released, handle the release
    intensityButtonPressed = false;
    mymotor->cleanUpAfterSlowOpen();
    notifyServer(10);
  }
}

// Function to handle open button press
void handleOnOffButtonPress()
{

  currentState = digitalRead(onOffButton);
  if (currentState == HIGH)
  {
    Serial.println(" handleOnoffButtonPress..");
    pressedTime = millis();

    // setup button to start blueTooth
    if (digitalRead(intensityButton) == HIGH)
    {
      Serial.println("Bluetooth Scann to be started ...");
      if (!bleInst->isConnected())
      {
        bleInst->doScan = true;
        bleStartMillis = millis();
      }
      else
      {
        Serial.println(" starting WIFI server");
        // start wifi service and turn off after 5 mins;
        if (!turnOnWiFi())
        {
          blink(BUILTINPIN, 5, 100);
        }
      }
      return;
    }

    // Button pressed
    // Wait for button release
    while (digitalRead(onOffButton) == HIGH)
    {
      delay(10);
    }
    pressDuration = millis() - pressedTime;
    if (pressDuration >= SHORT_PRESS_TIME)
    {
      // set the high level based on the current angle;
      if (mymotor->isBlindOpen())
      {
        Serial.println(" *****Setting the max angles");
        mymotor->setOpeningAngle();
      }
    }
    else
    {
      mymotor->openOrCloseBlind();
    }
    notifyServer(2);
  }
}
// broadasting info based on all prime number;
// default is 'blindName' if we intend to add more than one blind
// status: when divisible by 2,
// limits flag: when divisible by 3,
// SliderPosition when divisible by 5

void notifyLog(String message)
{
  ws.textAll("LOG :" + message);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  String jsonResponse;
  String paramName;
  String servoPositions[4];
  int i;


  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    JsonDocument doc;
    deserializeJson(doc, data);
    String jsonString ;
    serializeJson(doc, jsonString);
    String action = doc["action"];
    
    if (action == "getStatus")
    {
      JsonDocument response;
      response["action"] = "status";
      response["initialized"] = isInitialized;
      if (isInitialized)
      {
        response["blindsName"] = mymotor->getBlindName();
        response["servoCount"] = mymotor->getServoCount();
        JsonArray servos = response["servoPositions"].to<JsonArray>();
        for (i = 0; i < mymotor->getServoCount(); i++)
        {
          servos.add(servoPositions[i]);
        }
      }
      serializeJson(response, jsonResponse);
      ws.textAll(jsonResponse);
    }
    else if (action == "submit")
    {
      
      int *dir;
      JsonDocument response;     
      JsonObject data = doc["data"];
      if(DEBUG) {
        Serial.println(" Submit :" + jsonString);
      }

      String Name = data["blindsName"].as<String>();
      int counts = data["servoCount"].as<int>();
      dir = new int[counts];
      if (!isInitialized) {

        if (DEBUG) {
          Serial.println("submit 1:" + String(counts));
        }

        for (int i = 0; i < counts; i++)
        {
          paramName = "servo" + String(i) + "Position";
          servoPositions[i] = data[paramName].as<String>();

          servoPositions[i].toLowerCase();
          dir[i] = (servoPositions[i] == "left"? -1 : 1 );
          if(DEBUG) {
            Serial.println(" submit 2: " + String(i));
          }
        }


        // If mymotor exists, delete the old one
        if (mymotor != nullptr)
        {
            delete mymotor;
            mymotor = nullptr;
        }
        Serial.println("Submit: initialize-start...");

        // create motor object with relevant # of servos
        mymotor = new motorObj(counts, dir);

        if(DEBUG) Serial.println("setting -BlindName..");
        mymotor->setBlindName(Name);

        // attach motor to the BLEObject
        Serial.println(" Attaching motor...");
        bleInst->setMotor(mymotor);

        Serial.println("Submit: initialized..");

      } else{
        // throw error to the submit saying it has already been initialized
        // you might want to do factoryreset
        response["action"] = "submitResponse";
        response["message"] = "PreInitialized, if you want to change use factoryReset button";
        return;
      }
      isInitialized = true;
      
      // Save configuration to EEPROM or flash memory here
      mymotor->saveMotorParameters();

      response["action"] = "submitResponse";
      response["message"] = "Configuration updated successfully";
      String jsonResponse;
      serializeJson(response, jsonResponse);
      ws.textAll(jsonResponse);
      if(DEBUG) Serial.println(" submit : done..");
    }
    else if (action == "factoryReset")
    {
      String password = doc["password"];
      if (password == "XYZ123")
      {
        // Perform reset operations here
        isInitialized = false;
        mymotor->FactoryReset();
        // Clear EEPROM or flash memory here

        JsonDocument response;
        response["action"] = "factoryResetResponse";
        response["message"] = "Factory reset completed successfully";
        String jsonResponse;
        serializeJson(response, jsonResponse);
        ws.textAll(jsonResponse);
      }
      else
      {
        JsonDocument response;
        response["action"] = "factoryResetResponse";
        response["message"] = "Incorrect password";
        String jsonResponse;
        serializeJson(response, jsonResponse);
        ws.textAll(jsonResponse);
      }
    }
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    // New WebSocket client connected
    if (DEBUG)
      Serial.println("WebSocket client connected");
    client->text("LOG : WebSocket client connected !!! NEW " + String(ws.count()));
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    // WebSocket client disconnected
    if (DEBUG)
      Serial.println("WebSocket client disconnected");
    notifyError(client, "WebSocket client disconnected :" + String(ws.count()));
  }
  else if (type == WS_EVT_DATA)
  {
    handleWebSocketMessage(arg, data, len);
  }
}

void serverSetup()
{
  // Serial.println("Server Setup in progress...");
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);
  // Route for serving the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
}

void initiate_buttons()
{
  pinMode(onOffButton, INPUT_PULLDOWN);
  pinMode(intensityButton, INPUT_PULLDOWN);
  digitalWrite(onOffButton, LOW);
  digitalWrite(intensityButton, LOW);
}

void onOTAEnd(bool success)
{
  // Log when OTA has finished
  if (success)
  {
    Serial.println("OTA update finished successfully!");
  }
  else
  {
    Serial.println("There was an error during OTA update!");
  }
  // <Add your own code here>
}

void setOTA()
{
  // set up OTA
  ElegantOTA.begin(&server);
  ElegantOTA.setAuth("Admin", "admin");
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
}

void setup()
{
  pinMode(BUILTINPIN, OUTPUT);

  blink(BUILTINPIN, 1, 1000);
  bleInst = new bleClientObj();
  Serial.begin(115200);

  // start WIFI;
  startWifi();

  // initialize filesystem
  initFS();
  blink(BUILTINPIN, 2, 200);

  server.serveStatic("/", LittleFS, "/");
  blink(BUILTINPIN, 2, 1000);

  // setup the server
  serverSetup();
  blink(BUILTINPIN, 4, 200);

  setOTA();
  blink(BUILTINPIN, 2, 1000);

  // create motor instance;
  mymotor = new motorObj();
  if (mymotor->getBlindName() != "") {
    Serial.println(" BlindName : " + mymotor->getBlindName());
    isInitialized=true;
    // attach it to bleInst;
    bleInst->setMotor(mymotor);
  }
  Serial.println("isInitialized = " + String(isInitialized));

  server.begin();
  blink(BUILTINPIN, 5, 200);

  initiate_buttons();
  blink(BUILTINPIN, 6, 200);
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
      bleInst->doScan = false;
      // if connected shutdown the wifi
    }
    bleInst->doConnect = false;
  }
  else if (bleInst->doScan)
  {
    blink(BUILTINPIN, 2, 500);
    Serial.println(" Scanning ... again...");
    bleInst->scan();
    delay(1000);
    if ((millis() - bleStartMillis) > 120000)
    {
      bleInst->doScan = false;
    }
  }
  if (isWifiOn() && (millis() - wifiStartMillis) > 300000)
  {
    if (turnOffWiFi())
    {
      Serial.println("Wifi Turned off...");
      blink(BUILTINPIN, 5, 100);
    }
  }
  delay(500);
}