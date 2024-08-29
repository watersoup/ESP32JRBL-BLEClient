#include <Arduino.h>
#include <bleClientObj.h>
#include <BLEDevice.h>
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


void setup()
{
    bleInst = new bleClientObj();
    pinMode(BUILTINPIN,OUTPUT);
    blink(BUILTINPIN,2,1000);
    Serial.begin(115200);
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