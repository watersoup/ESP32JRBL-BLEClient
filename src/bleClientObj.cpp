#include <bleClientObj.h>

// constructor
bleClientObj::bleClientObj(){

    /* Specify the Characteristic UUID of Server */

    doConnect = false;
    connected = false;
    doScan = false;
    myDevice = nullptr;
    const int BUILTINPIN = 8;
    std::string g_rxValue;
    String g_txValue;
    BLERemoteCharacteristic *pRemoteChar = nullptr;
    charUUID = nullptr;
    serviceUUID = BLEUUID(SERVICE_UUID);
    charUUIDarr[0] = BLEUUID("7fac1651-859b-4860-96b9-da21b4205ad9");
    charUUIDarr[1] = BLEUUID("7fac1651-859b-4861-96b9-da21b4205ad9");
    charUUIDarr[2] = BLEUUID("7fac1651-859b-4862-96b9-da21b4205ad9");
    charUUIDarr[3] = BLEUUID("7fac1651-859b-4863-96b9-da21b4205ad9");
    charUUIDarr[4] = BLEUUID("7fac1651-859b-4864-96b9-da21b4205ad9");
    charUUIDarr[5] = BLEUUID("7fac1651-859b-4865-96b9-da21b4205ad9");
    

    // initialize the BLE device;
    BLEDevice::init("JRcl");

}
//destructor

void setMotor(motorObj *motor){
    motor = motor;
}

// start the ble should be called when we are ready 
void bleClientObj::scan(){
    /* Retrieve a Scanner and set the callback we want to use to be informed when we
      have detected a new device.  Specify that we want active scanning and start the
      scan to run for 5 seconds. */
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks( this));
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(false);
    pBLEScan->start(15, false);
    Serial.println(" Started active Scan...");
}
// this runs as soon as it finds an adversing server
void bleClientObj::onScanResult(BLEAdvertisedDevice advertisedServer){
    Serial.print("BLE Advertised Device found: ");
    Serial.print(advertisedServer.toString().c_str());Serial.print( "->");
    Serial.println(advertisedServer.haveServiceUUID() ? "YES": "NO---");

    /* We have found a device, let us now see if it contains the service we are looking for. */
    if (advertisedServer.haveServiceUUID() && (advertisedServer.isAdvertisingService(serviceUUID) || 
        advertisedServer.getServiceUUID().equals(serviceUUID) ) )
    {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedServer);
        doConnect = true;
        doScan = false;
    } else {
    Serial.println(" found but not relevant to us :"+ String(advertisedServer.getAddress().toString().c_str()));
    }
}
/* Called for each advertising BLE server. */
void bleClientObj::MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice advertisedDevice)
{
    ParentObj->onScanResult(advertisedDevice);
}

// process comands from the Server;
void bleClientObj::processCommand(const String& command) {
    if (command == "OPEN") {
        Serial.println("Opening blinds");
        // Add code to open blinds
        if (motor != nullptr) {
            motor->openBlinds();
        }
    } else if (command == "CLOSE") {
        Serial.println("Closing blinds");
        // Add code to close blinds
        if (motor != nullptr) {
            motor->closeBlinds();
        }
    } else if (command.startsWith("WINDOWMAX:")) { // of limit setting
        int position = command.substring(13).toInt();
        Serial.println("Setting position to: " + String(position) + "%");
        // Add code to set blinds position
        if (motor != nullptr) {
            motor->setWindowMax(motor->getPositionOfMotor(position));
        }
    } else if (command.startsWith("WINDOWMIN:")) { // of limit setting
        int position = command.substring(13).toInt();
        Serial.println("Setting position to: " + String(position) + "%");
        // Add code to set blinds position
        if (motor != nullptr) {
            motor->setWindowLow(motor->getPositionOfMotor(position));
        }
    }else if (command.startsWith("SLIDERPOSITION:")){ //slider moved
        int position = command.substring(16).toInt();
        Serial.println("Recd slider position: " + String(position) + "%");
        if (motor != nullptr){
            motor->moveBlinds(motor->getPositionOfMotor(position));
        }
    
    }else {
        Serial.println("Unknown command: " + command);
    }
}

void bleClientObj::setMotor(motorObj *m) {
    motor = m;
}

bool bleClientObj::connectCharacteristic(BLERemoteService *pRemoteService, BLEUUID l_CharUUID){
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
      pRemoteCharacteristic->registerForNotify(
                [this](BLERemoteCharacteristic *pRemoteCharacteristic, uint8_t* pData, 
                            size_t length, bool isNotify) { 
                    this->notifyCallback(pRemoteCharacteristic, pData, length, isNotify);
                   });
    return true;
}

/* if server tries to contact and notify certain thing this is where
    it comes you can tackle the notification base*/
void bleClientObj::notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify)
{

    if ( pBLERemoteCharacteristic->getUUID().equals(*charUUID)){
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
}
/* Start connection to the BLE Server */
bool bleClientObj::connectToServer()
{
    if ( !connected && doConnect ){
        Serial.print("Forming a connection to ");
        Serial.println(myDevice->getAddress().toString().c_str());
        
        BLEClient*  pClient  = BLEDevice::createClient();
        Serial.println(" - Created client");

        pClient->setClientCallbacks(new MyClientCallback(this));

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

        for ( int i=0 ; i < 6; i ++){
            connected = connectCharacteristic(pRemoteService, charUUIDarr[i] );
            if (connected == true) {
                charUUID = &(charUUIDarr[i]);
                Serial.println("Connected to character = " + String(i));
                break;
            }
        }
        if ( connected == false){
            pClient->disconnect();
            Serial.println("At least one charactersitic UUID not found");
            return false;
        }

        pRemoteChar = pRemoteService->getCharacteristic(*charUUID);
    }

    return connected;
}

void bleClientObj::MyClientCallback::onConnect(BLEClient* pclient)
{
    Serial.println("onConnect \n");
    ParentObj->connected = true;
}
void bleClientObj::MyClientCallback::onDisconnect(BLEClient* pclient)
{
    ParentObj->connected = false;
    Serial.println("onDisconnect");
    ParentObj->doScan=true;
}
bool bleClientObj::isConnected(){
    return connected;
}
// this will read in the status on request but if it is notified then
// it will go to notify, so bascially one can call notify from this function
void bleClientObj::readStatus(){

}
void bleClientObj::writeStatus(const String msg){

    Serial.println(" WRITE STATUS: " + msg);
    pRemoteChar->writeValue(msg.c_str(), msg.length());
    delay(1000);
}

// broadasting info based on all prime number;
// default is 'blindName' if we intend to add more than one blind
// status: when divisible by 2,
// limits flag: when divisible by 3,
// SliderPosition when divisible by 5
void bleClientObj::notifyBLEServer(int x)
{
    String msg;
    msg = "blindName :" + motor->getBlindName();
    writeStatus(msg);
    int openflag = motor->isBlindOpen() ? 1 : 0;
    int limitflag = motor->getLimitFlag();
    int pos = motor->getPositionOfSlider();
    if (x % 2 == 0)
    {
        msg = "status :"+ String(openflag);
        writeStatus(msg);
    }
    if (x % 3 == 0)
    {
        msg = "limitSetupFlag :" + String(limitflag);
        writeStatus(msg);
    }
    if (x % 5 == 0)
    {
        msg = "sliderPosition :" + String(pos);
        writeStatus(msg);
    }
}