#ifndef SERVICE_UUID
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-CBA987654321"
#endif


#ifndef BLECLIENT_OBJ_H
#define BLECLIENT_OBJ_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEClient.h>
#include <BLE2902.h>
#ifndef MOTOR_OBJ_H
#include <motorObj.h>
#endif

struct clientInfoPkg {
    uint16_t connId;
    String blindName;
    long int currentSliderPosition;
    String status;
    int limitStatus;
};

class bleClientObj {
public:
    bleClientObj();
    void configSecurity();
    void scan();
    bool connectToServer();
    bool isConnected();
    void readStatus();
    void writeStatus(const String msg);
    void onScanResult(BLEAdvertisedDevice advertisingServer);
    bool connectCharacteristic(BLERemoteService*, BLEUUID);
    void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                        uint8_t *pData, size_t length, bool isNotify);
    clientInfoPkg getInfo();
    void processCommand(const String& command);
    
    void setMotor(motorObj *motor);
    // notify server about the status of the blinds
    // if it is changed locally through BLE
    void notifyBLEServer( int x=1);

    boolean doConnect =false;
    boolean connected = false;
    boolean doScan = false;
    boolean recdDataFlag = false;

    String receivedData="";
private:
    motorObj *motor;
    BLEUUID serviceUUID;
    /* Specify the Characteristic UUID of Server  at a time only 6 */
    BLEUUID charUUIDarr[6], *charUUID;


    BLEAdvertisedDevice* bleServerHub;
    const int BUILTINPIN = 8;

    // Only one characteristics should be connected at most
    
    BLERemoteCharacteristic *pRemoteChar = nullptr;
    BLEClient *pClient = nullptr;

    /* Scan for BLE servers and find the first one that advertises the service we are looking for. */
    class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks
    {
        bleClientObj* ParentObj;
        public:
        MyAdvertisedDeviceCallbacks(bleClientObj* clientObj): ParentObj(clientObj) {}
        /* Called for each advertising BLE server. */
        void onResult(BLEAdvertisedDevice advertisedDevice);
    };

    class MyClientCallback : public BLEClientCallbacks
    {
        bleClientObj *ParentObj;
        public:
        MyClientCallback(bleClientObj * clientObj): ParentObj(clientObj){}
        void onConnect(BLEClient *);
        void onDisconnect(BLEClient *);
    };

};

class MySecurity : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() {
        Serial.println("PassKeyRequest");
        return 123456; // Set your passkey here
    }

    void onPassKeyNotify(uint32_t pass_key) {
        Serial.println("On passkey Notify number: " + String(pass_key));
    }

    bool onConfirmPIN(uint32_t pass_key) {
        Serial.println("Confirming PIN: " + String(pass_key));
        return true;
    }

    bool onSecurityRequest() {
        Serial.println("Security Request");
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
        if (cmpl.success) {
            Serial.println("Pairing success");
        } else {
            Serial.println("Pairing failed");
        }
    }
};
#endif  //BLECLIENT_OBJ_H