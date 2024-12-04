#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WiFi.h>
#ifndef ESPAsyncWiFiManager_h
#include <ESPAsyncWebServer.h>
#endif
#include <ESPAsyncWiFiManager.h>
#include <ElegantOTA.h>
#include <BLEDevice.h>
#include <bleClientObj.h>
#include <LittleFSsupport.h>
#include "motorObj.h"
// Undefine DEBUG if previously defined to avoid conflicts
#ifdef DEBUG
#undef DEBUG
#endif

// Define DEBUG and various timing constants
#define DEBUG 1
#define SHORT_PRESS_TIME 500      // Short press duration in milliseconds
#define LONG_PRESS_TIME 2000      // Long press duration in milliseconds
#define MIN_PRESS_TIME 200        // Minimum press duration to be considered valid
#define SPIFFS LittleFS            // Define SPIFFS as LittleFS
#define CLOSEBLINDS 0             // Command identifier for closing blinds
#define OPENBLINDS 1              // Command identifier for opening blinds

// Initialize AsyncWebServer on port 80
AsyncWebServer server(80);
// Initialize DNS Server for async WiFi management
DNSServer dnsServer;
// Initialize WebSocket endpoint
AsyncWebSocket ws("/ws");

// Instantiate AsyncWiFiManager with server and DNS server
// AsyncWiFiManager *wifiManager = new AsyncWiFiManager(&server, &dnsServer);

// Timing variables for OTA and WiFi operations
unsigned long ota_progress_millis = 0;
unsigned long wifiStartMillis = 0;
unsigned long bleStartMillis = 0;
unsigned long lastUpdateTime =0, lastBlinkTime=0;

// Define pin numbers
int BUILTINPIN = 8;            // Built-in LED pin
const int onOffButton = 20;    // On/Off button pin
const int intensityButton = 21;// Intensity button pin

// Variables to manage button states and durations
int currentState;
long int pressedTime, pressDuration;

// BLE advertising timing
unsigned long advertisingStartTime = 0;
const unsigned long advertisingTimeout = 180000; // 180 seconds

// Flags for button presses and initialization status
bool intensityButtonPressed = false;
bool isInitialized = false;

// Counter variable
int count = 0;

// BLE client instance
bleClientObj *bleInst = nullptr;

// Variables for BLE communication
std::string g_rxValue;
String g_txValue;

// Motor object instance with servos
motorObj *mymotor;

// Function to blink an LED a specified number of times with a frequency
void blink(const int PIN, int times = 1, long int freq = 500)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(PIN, LOW);    // Turn LED on
    delay(freq);               // Wait
    digitalWrite(PIN, HIGH);   // Turn LED off
    delay(freq);               // Wait
  }
}

// Function to notify all WebSocket clients of an error message
void notifyError(String message)
{
  ws.textAll("LOG : " + message);
}

// Overloaded function to notify a specific WebSocket client of an error message
void notifyError(AsyncWebSocketClient *client, String message)
{
  client->text("LOG : " + message);
}

// Function to check if WiFi is currently enabled
bool isWifiOn()
{
  if (WiFi.getMode() != WIFI_OFF)
  {
    return true;
  }
  return false;
}


void connectToWifi() {

  // go in loop till its not connected
  while (WiFi.status() != WL_CONNECTED) {
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    AsyncWiFiManager wifiManager(&server,&dnsServer);


    if (!wifiManager.startConfigPortal("BleClt-AP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.restart();
      delay(5000);
    }

    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Serial.println(WiFi.localIP());
  }
}
// Function to turn on WiFi
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
  
  // Start the timer
  wifiStartMillis = millis();
  return true;
}

// Function to turn off WiFi
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

// OTA event handlers

// Function called when OTA update starts
void onOTAStart()
{
  Serial.println("OTA update started!");
  // <Add your own code here>
}

// Function called to indicate OTA progress
void onOTAProgress(size_t current, size_t final)
{
  // Log every 1 second
  if (millis() - ota_progress_millis > 1000)
  {
    ota_progress_millis = millis();
    Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
  }
}

// Function to notify the server with the current status
void notifyServer(int x = 1)
{
  JsonDocument jsonDoc;
  String jsonString;

  // Populate JSON with motor status information
  jsonDoc["blindName"] = mymotor->getBlindName();
  int openflag = mymotor->isBlindOpen() ? 1 : 0;
  int limitflag = mymotor->getLimitFlag();
  int pos = mymotor->getCurrentSliderPosition();
  if ( limitflag == 3){
    // put off wifi as the setup is done;
    turnOffWiFi();

  }
  // Conditionally add fields based on divisibility of x
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

  jsonDoc.shrinkToFit(); // Optimize memory usage
  serializeJson(jsonDoc, jsonString); // Serialize JSON to string

  // Debugging output
  if (DEBUG)
    Serial.println(" NotifyClient " + String(x) + " :" + jsonString);

  if (ws.count() > 0)
    ws.textAll(jsonString); // Send JSON to all WebSocket clients

  // If BLE server is active, notify through BLE object as well
  if (bleInst != nullptr && bleInst->isConnected())
    bleInst->notifyBLEServer(x);

  mymotor->saveMotorParameters(); // Save motor parameters to EEPROM
  lastUpdateTime = millis(); // Update the last update time
  delay(5 * x);              // Delay based on parameter x
}

// Function to handle both button pushes (On/Off and Intensity)
void handleBothButtonPush()
{
  mymotor->loadMotorParameters(); // Load motor parameters
  Serial.printf("servoCount = %d\n", mymotor->getServoCount());

  Serial.println("Bluetooth Scan to be started ...");
  if (!bleInst->isConnected())
  {
    bleInst->doScan = true; // Start BLE scanning if not connected
  }
  else
  {
    // Start WiFi service and turn off after 5 minutes
    Serial.println(" starting WIFI server");
    if (!turnOnWiFi())
    {
      blink(BUILTINPIN, 5, 100); // Indicate failure with LED blink
    }
  }
  intensityButtonPressed = false; // Reset the intensity button pressed flag
}

// Function to handle intensity button press
void handleIntensityButtonPress()
{
  pressedTime = millis(); // Record the time when button was pressed

  // Check if intensity button is pressed
  if (digitalRead(intensityButton) == HIGH)
  {
    delay(100); // Debounce delay

    // Check if On/Off button is also pressed
    if (digitalRead(onOffButton) == HIGH)
    {
      handleBothButtonPush();
      return;
    }

    if (!intensityButtonPressed){
      mymotor->attachAll(); // Attach all servos if not already attached
      delay(1000);
      Serial.println("Intensity Button pressed....slow opening/closing");
    }

    // Handle the slow movement of servos
    mymotor->slowMove();
    delay(30); // Short delay
    intensityButtonPressed = true; // Set the pressed flag
  }
  else if (intensityButtonPressed)
  {
    Serial.println("Intensity button released...handle the flags");
    // Handle the release of the intensity button
    intensityButtonPressed = false;
    mymotor->cleanUpAfterSlowMove(); // Cleanup after movement
    notifyServer(10); // Notify server with x=10
  }
}

// Function to handle On/Off button press
void handleOnOffButtonPress()
{
  currentState = digitalRead(onOffButton); // Read the current state of the On/Off button
  if (currentState == HIGH)
  {
    pressedTime = millis(); // Record the time when button was pressed
    delay(100); // Debounce delay

    // Check if Intensity button is also pressed
    if (digitalRead(intensityButton) == HIGH)
    {
      handleBothButtonPush();
      return;
    }
    Serial.println(" handleOnoffButtonPress..");

    // Wait for button release
    while (digitalRead(onOffButton) == HIGH)
    {
      delay(50);
    }
    pressDuration = millis() - pressedTime; // Calculate press duration

    if (pressDuration >= SHORT_PRESS_TIME)
    {
      // Set the angle based on whether blinds are open or closed
      if (!mymotor->isBlindOpen())
      {
        if(DEBUG) Serial.println(" *****Setting the max angles");
        mymotor->setOpeningAngle();
        mymotor->openBlinds();
      }
      else
      {
        if(DEBUG) Serial.println(" *****Setting the min angles");
        mymotor->setClosingAngle();
        mymotor->closeBlinds();
      }
    }
    else
    {
      mymotor->openOrCloseBlind(); // Toggle the blinds
    }
    notifyServer(10); // Notify server with x=2
  }
}

// Function to notify log messages to WebSocket clients
void notifyLog(String message)
{
  ws.textAll("LOG :" + message);
}

// Function to handle incoming WebSocket messages
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg; // Cast the argument to AwsFrameInfo
  String jsonResponse;
  String paramName;
  int i;

  // Check if the message is a complete final text frame
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    JsonDocument doc;
    deserializeJson(doc, data); // Deserialize the incoming JSON data
    String jsonString;
    serializeJson(doc, jsonString); // Serialize it back to string for debugging
    String action = doc["action"];  // Extract the action field

    if (action == "OPEN")
    {
      mymotor->openBlinds(); // Open the blinds
      notifyServer(10);       // Notify server with x=3
    }
    else if (action == "CLOSE")
    {
      mymotor->closeBlinds(); // Close the blinds
      notifyServer(10);        // Notify server with x=4
    }else if (action == "setupLimits"){
      if(mymotor->setLimits())
        notifyServer(10); // Notify server with x=5
    }
    else if (action == "getStatus")
    {
      JsonDocument response;
      response["action"] = "status";
      response["initialized"] = isInitialized;

      if (isInitialized)
      {
        // If initialized, include blinds name and servo count
        response["blindsName"] = mymotor->getBlindName();
        response["servoCount"] = mymotor->getServoCount();
        response["limitFlag"] = mymotor->getLimitFlag();
        response["status"] = (mymotor->isBlindOpen()?1:0);
        response["sliderPosition"] = mymotor->getCurrentSliderPosition();
        // Create a JSON array for servo directions
        JsonArray servos = response["Direction"].to<JsonArray>();
        int *dirInt = mymotor->getDirections();
        String dir = "";

        for (i = 0; i < mymotor->getServoCount(); i++)
        {
          // Convert direction integers to strings
          dir = (dirInt[i] == 1 ? "Right" : "Left");
          servos.add(dir);
        }
      }

      serializeJson(response, jsonResponse); // Serialize the response
      if (DEBUG)
        Serial.println(" getStatus : " + jsonResponse);

      ws.textAll(jsonResponse); // Send the response to all WebSocket clients
    }
    else if (action == "submit")
    {
      int *dirVec;
      JsonDocument response;
      JsonObject data = doc["data"];

      if (DEBUG)
      {
        Serial.println(" Submit :" + jsonString);
      }

      int counts = data["servoCount"].as<int>(); // Get the number of servos
      dirVec = new int[counts];                   // Allocate memory for direction vector

      if (!isInitialized)
      {
        // Retrieve servo positions from the submitted data
        for (int i = 0; i < counts; i++)
        {
          paramName = "servo" + String(i + 1) + "Position";
          // Assign direction based on position ("left" or "right")
          dirVec[i] = data[paramName].as<String>() == "left" ? -1 : 1;
          Serial.printf("dirVec[%d] = %s - %d\n", i, data[paramName].as<String>(), dirVec[i]);
        }

        // If mymotor exists, delete the old instance
        if (mymotor != nullptr)
        {
          delete mymotor;
          mymotor = nullptr;
        }

        // Create a new motor object with the specified number of servos and directions
        mymotor = new motorObj(counts, dirVec);
        Serial.println("Submit: Created Motor object ");

        // Set the name of the blinds
        mymotor->setBlindName(data["blindsName"].as<String>());

        // Attach the motor to the BLE object for communication
        Serial.println(" Attaching motor...");
        bleInst->setMotor(mymotor);

        Serial.println("Submit: done...");
        delete[] dirVec; // Free allocated memory
      }
      else
      {
        // If already initialized, send an error response
        response["action"] = "submitResponse";
        response["message"] = "PreInitialized, if you want to change use factoryReset button";
        serializeJson(response, jsonResponse); // Serialize the response
        ws.textAll(jsonResponse);               // Send the response
        return;                                 // Exit the function
      }

      isInitialized = true; // Mark as initialized

      // Send a success response
      response["action"] = "submitResponse";
      response["message"] = "Configuration updated successfully";
      serializeJson(response, jsonResponse);
      ws.textAll(jsonResponse);

      if (DEBUG)
        Serial.println(" submit : done..");
    }
    else if (action == "factoryReset")
    {
      String password = doc["password"]; // Get the password from the request
      if (password == "XYZ123")          // Verify the password
      {
        // Perform reset operations
        isInitialized = false;
        mymotor->FactoryReset(); // Factory reset the motor

        // Clear EEPROM or flash memory here (implementation depends on motorObj)

        // Send a success response
        JsonDocument response;
        response["action"] = "factoryResetResponse";
        response["message"] = "Factory reset completed successfully";
        String jsonResponse;
        serializeJson(response, jsonResponse);
        ws.textAll(jsonResponse);

        isInitialized = false; // Ensure initialized flag is reset
        ESP.restart();         // Restart the ESP32
      }
      else
      {
        // Send an error response for incorrect password
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

// WebSocket event handler
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
    handleWebSocketMessage(arg, data, len); // Handle incoming WebSocket data
  }
}

// Function to set up the web server and WebSocket handlers
void serverSetup()
{
  // Add WebSocket handler to the server
  server.addHandler(&ws);
  ws.onEvent(onWsEvent);

  // Route for serving the HTML page
  server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/setup.html", "text/html"); });
  // Route for serving the HTML page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
}

// Function to initialize button pins
void initiate_buttons()
{
  pinMode(onOffButton, INPUT_PULLDOWN);      // Set On/Off button as input with pull-down resistor
  pinMode(intensityButton, INPUT_PULLDOWN);  // Set Intensity button as input with pull-down resistor
  digitalWrite(onOffButton, LOW);            // Ensure On/Off button is LOW
  digitalWrite(intensityButton, LOW);        // Ensure Intensity button is LOW
}

// Function called when OTA update ends
void onOTAEnd(bool success)
{
  // Log the result of the OTA update
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

// Function to set up OTA functionalities
void setOTA()
{
  ElegantOTA.begin(&server); // Initialize ElegantOTA with the server
  ElegantOTA.setAuth("Admin", "admin"); // Set authentication for OTA
  ElegantOTA.onStart(onOTAStart);      // Set callback for OTA start
  ElegantOTA.onProgress(onOTAProgress); // Set callback for OTA progress
  ElegantOTA.onEnd(onOTAEnd);          // Set callback for OTA end
}

// Arduino setup function
void setup()
{
  pinMode(BUILTINPIN, OUTPUT); // Set built-in LED pin as output
  blink(BUILTINPIN, 1, 200); // Blink LED twice with 200ms interval

  Serial.begin(115200); // Initialize serial communication at 115200 baud


  // Start WiFi connection
  connectToWifi();


  // Initialize filesystem
  initFS();
  blink(BUILTINPIN, 2, 200); // Blink LED twice with 200ms interval


  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/");
  // server.serveStatic("/setup.css", LittleFS, "/setup.css");
  // server.serveStatic("/setup.js", LittleFS, "/setup.js");
  blink(BUILTINPIN, 2, 1000); // Blink LED twice with 1-second interval

  blink(BUILTINPIN, 1, 1000); // Blink LED once with 1-second interval
  bleInst = new bleClientObj(); // Create a new BLE client instance
  
  // Set up the web server and WebSocket handlers
  serverSetup();
  blink(BUILTINPIN, 4, 200); // Blink LED four times with 200ms interval

  // Set up OTA functionalities
  setOTA();
  blink(BUILTINPIN, 2, 1000); // Blink LED twice with 1-second interval

  // Create motor instance
  mymotor = new motorObj();
  if (mymotor->isInitialized())
  {
    Serial.println(" BlindName : " + mymotor->getBlindName());
    isInitialized = true;
    // Attach motor to BLE client object
    bleInst->setMotor(mymotor);
  }
  Serial.println("isInitialized = " + String(isInitialized));

  server.begin(); // Start the web server
  blink(BUILTINPIN, 5, 200); // Blink LED five times with 200ms interval

  initiate_buttons(); // Initialize button pins
  blink(BUILTINPIN, 6, 200); // Blink LED six times with 200ms interval
}

// Arduino loop function
void loop()
{
  // Check for On/Off button press
  handleOnOffButtonPress();

  // Check if Intensity button is pressed
  handleIntensityButtonPress();

  /* 
     If the flag "doConnect" is true, then a desired BLE Server has been found.
     Connect to it. Once connected, set the connected flag to true.
  */
  if (bleInst->doConnect == true)
  {
    if (bleInst->connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
      bleInst->doScan = false; // Stop scanning once connected
      // Optionally, shutdown WiFi if connected via BLE
    }
    bleInst->doConnect = false; // Reset the connect flag
  }
  else if (bleInst->doScan)
  {
    blink(BUILTINPIN, 2, 500); // Blink LED twice with 500ms interval
    Serial.println(" Scanning ... again...");
    bleInst->scan();           // Start BLE scanning
    delay(1000);                // Wait for a second
    if ((millis() - bleStartMillis) > 120000) // Check if scanning time exceeded 2 minutes
    {
      bleInst->doScan = false; // Stop scanning after timeout
    }
  }

  // Manage WiFi connection: Turn off WiFi after 20 minutes of being on
  if (isWifiOn() && (millis() - wifiStartMillis) > 1200000)
  {
    if (turnOffWiFi())
    {
      Serial.println("Wifi Turned off...");
      blink(BUILTINPIN, 5, 100); // Blink LED five times with 100ms interval
    }
  }

  // Process received data from BLE
  if (bleInst->recdDataFlag)
  {
    bleInst->processCommand(bleInst->receivedData); // Process the received command
    bleInst->recdDataFlag = false;                 // Reset the received data flag
    bleInst->receivedData = "";                     // Clear the received data
    notifyServer(10);                               // Notify server with x=10
  }
  if ( (millis() - lastBlinkTime) > 5000)
  {
    blink(BUILTINPIN, 1, 100); // Blink LED once with 100ms interval
    lastBlinkTime = millis();
  }
  
}