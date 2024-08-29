#include <Arduino.h>

#include <BLEDevice.h>

int count =0;
void blink(const int PIN, int times=1, long int freq=500){
  for(int i = 0; i < times; i++){
    digitalWrite(PIN, LOW);
    delay(freq);
    digitalWrite(PIN, HIGH);
    delay(freq);
  }
}

/* Specify the Service UUID of Server */
static BLEUUID serviceUUID("155bbbb6-c317-4e5d-ac5f-9d76897df05f");
/* Specify the Characteristic UUID of Server */
static BLEUUID    charUUID_1("7fac1651-859b-4860-96b9-da21b4205ad9");
static BLEUUID    charUUID_2("7fac1651-859b-4861-96b9-da21b4205ad9");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLEAdvertisedDevice* myDevice;
const int BUILTINPIN = 8;
std::string g_rxValue;
String g_txValue;
BLERemoteCharacteristic *pRemoteChar_1 = nullptr;
BLERemoteCharacteristic *pRemoteChar_2 = nullptr;

static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify)
{

    if ( pBLERemoteCharacteristic->getUUID().equals(charUUID_1)){
        // Serial.print(" UUID 1 of data length ");
        // Serial.println(length);
        // uint32_t counter = pData[0];
        // for(int i =1; i <length; i++){
        //   counter = counter | (pData[i] <<i*8);
        // }
        Serial.print("RCD-1:");
        for (int i = 0 ; i < length; i++){
            Serial.print((char) pData[i]);
        }
        Serial.println(".");
    } 
    else if (pBLERemoteCharacteristic->getUUID().equals(charUUID_2)){
        Serial.print("RCD-2:");
        for (int i = 0 ; i < length; i++){
            Serial.print((char) pData[i]);
        }
        Serial.println(".");
    }

}

class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient* pclient)
    {
        Serial.println("onConnect \n");
        connected = true;
    }

    void onDisconnect(BLEClient* pclient)
    {
        connected = false;
        Serial.println("onDisconnect");
        doScan=true;
    }

};

bool connectCharacteristic(BLERemoteService *pRemoteService, BLEUUID l_CharUUID){
    /* Obtain a reference to the characteristic in the service of the remote BLE server */
    BLERemoteCharacteristic *pRemoteCharacteristic;
    pRemoteCharacteristic = pRemoteService->getCharacteristic(l_CharUUID);
    if (pRemoteCharacteristic == nullptr)
    {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(l_CharUUID.toString().c_str());
      return false;
    }
    Serial.println(" - Found our characteristic");

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

  return true;
}

/* Start connection to the BLE Server */
bool connectToServer()
{
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
      
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

      /* Connect to the remote BLE Server */
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

      /* Obtain a reference to the service we are after in the remote BLE server */
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    connected = true;
    connected = connectCharacteristic(pRemoteService, charUUID_1  ) && connectCharacteristic(pRemoteService, charUUID_2);
    if ( connected == false){
        pClient->disconnect();
        Serial.println("At least one charactersitic UUID not found");
        return false;
    }
    pRemoteChar_2 = pRemoteService->getCharacteristic(charUUID_2);
    pRemoteChar_1 = pRemoteService->getCharacteristic(charUUID_1);
    return connected;
}

/* Scan for BLE servers and find the first one that advertises the service we are looking for. */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
{
    /* Called for each advertising BLE server. */
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.print("BLE Advertised Device found: ");
        Serial.print(advertisedDevice.toString().c_str());Serial.print( "->");
        Serial.println(advertisedDevice.haveServiceUUID() ? "YES": "NO---");

        /* We have found a device, let us now see if it contains the service we are looking for. */
        if (advertisedDevice.haveServiceUUID() && (advertisedDevice.isAdvertisingService(serviceUUID) || 
              advertisedDevice.getServiceUUID().equals(serviceUUID) ) )
        {
            BLEDevice::getScan()->stop();
            myDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        } else {
          Serial.println(" found but not relevant to us :"+ String(advertisedDevice.getAddress().toString().c_str()));
        }
    }
};

void start_ble(){
     BLEDevice::init("JRcl");
    /* Retrieve a Scanner and set the callback we want to use to be informed when we
      have detected a new device.  Specify that we want active scanning and start the
      scan to run for 5 seconds. */
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(false);
    pBLEScan->start(5, false);
    Serial.println(" Started active Scan...");

}
void setup()
{
    pinMode(BUILTINPIN,OUTPUT);
    blink(BUILTINPIN,2,1000);
    Serial.begin(115200);
    Serial.println("Starting Arduino BLE Client application...");
    start_ble();
    blink(BUILTINPIN,10,200);
}


void loop()
{

  /* If the flag "doConnect" is true, then we have scanned for and found the desired
     BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
     connected we set the connected flag to be true. */
    if (doConnect == true)
    {
        if (connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
            doScan=false;
            // if connected shutdown the wifi

        } 
        else
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
        doConnect = false;
    }

    /* If we are connected to a peer BLE Server, update the characteristic each time we are reached
      with the current time since boot */
    if (connected)
    {
        // g_rxValue = pRemoteChar_2->readValue();
        // Serial.println("read:" + String(g_rxValue.c_str()));
        
        /* Set the characteristic's value to be the array of bytes that is actually a string */
        int rand_num = -random(1000);
        g_txValue = "Cl-2:" + String(rand_num);
        Serial.println("---> " + String(rand_num));
        pRemoteChar_2->writeValue(g_txValue.c_str(), g_txValue.length());
        delay(1000);
        g_txValue = "Cl-1:" + String(rand_num);
        pRemoteChar_1->writeValue(g_txValue.c_str(), g_txValue.length());
        /* You can see this value updated in the Server's Characteristic */
        blink(BUILTINPIN,1, 500);
    }
    else if(doScan)
    {
        blink(BUILTINPIN,2, 500);
        Serial.println( " Scanning ... again...");
        BLEDevice::getScan()->start(5,false);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
    }
    delay(1000);
    
}