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
    void scan();
    bool connectToServer();
    bool isConnected();
    void readStatus();
    void writeStatus(const String msg);
    void onScanResult(BLEAdvertisedDevice advertisingServer);
    bool connectCharacteristic(BLERemoteService*, BLEUUID);
    void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                            uint8_t* pData, size_t length, bool isNotify);
    clientInfoPkg getInfo();

    boolean doConnect =false;
    boolean connected = false;
    boolean doScan = true;
private:

    BLEUUID serviceUUID;
    /* Specify the Characteristic UUID of Server  at a time only 6 */
    BLEUUID charUUIDarr[6], *charUUID;


    BLEAdvertisedDevice* myDevice;
    const int BUILTINPIN = 8;

    // Only one characteristics should be connected at most
    
    BLERemoteCharacteristic *pRemoteChar = nullptr;


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

#endif  //BLECLIENT_OBJ_H