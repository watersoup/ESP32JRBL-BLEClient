
#include <motorObj.h>
#define maxUs 2580 // for 180servo this is best
#define minUs  315 // min position for 180Servo;
#define ANGLE_INCREMENT 5
#define maxFdBk 4095
#define minFdBk 515
int spinlist[5] = {9,8,7,6,5};
int rpinlist[5] = {0, 1,2,3,4};



motorObj::motorObj(int numMotors): numServos(numMotors){

    // create sPin object
    sPin = new int[numServos];
    rPin = new int[numServos];
    direction = new int[numServos];

    for ( int k =0; k <numServos;k++){
        sPin[k] = spinlist[k];
        rPin[k] = rpinlist[k];
        direction[k]=1;
    }
    // initialize the sPin and rPin as per ESP32c3 super Mini by default;
    // Initialize pins
    myservo = new Pwm();
    for(int k=0; k<numServos;k++){
        pinMode(sPin[k], OUTPUT);
        pinMode(rPin[k], INPUT_PULLDOWN);
        myservo->attachServo(sPin[k],minUs, maxUs);
        delay(500);
        myservo->writeServo(sPin[k], SERVO_MAX_ANGLE);
        delay(1000);
        myservo->detach(sPin[k]);
        // direction[k] = 1; // set all on the same side;
    }
}
motorObj::~motorObj(){
    // destroy sPin and rPin;
    // destroy directions created;
    delete[] sPin;
    delete[] rPin;
    delete[] direction;
    delete myservo;
}
void motorObj::setDirections(int *dirs){

    if (sizeof(dirs)<numServos*sizeof(int)){
        Serial.println(" ERROR : size of the dir is smaller");
        return;
    }
    for (int k =0; k<numServos;k++){
        direction[k] = dirs[k];
    }
}

void motorObj::attachAll(){
    int k1;
    if( !servoAttached){
        for(int k=0;k<numServos;k++){
            k1 = myservo->attachServo(sPin[k], minUs, maxUs);
            if (k1 == 253 || k1 == 255){
                Serial.printf(" Couldn't Attach : %d \n",k1);
                delay(5000);
            }
        }
    }
    servoAttached = true;
}

void motorObj::detachAll(){
    if( servoAttached){
        for(int k=0; k < numServos;k++){
            myservo->detach(sPin[k]);
        }
    }
    servoAttached=false;
}
void motorObj::slowOpen(){
    int k1, pos_deg;
    // Button pressed, handle the press
    for(int k = 0; k < numServos; k++){
        k1 = myservo->attached(sPin[k]);
        pos_deg = myservo->read(sPin[k]);
        Serial.printf(".. posdeg : %d ch: %d \n", pos_deg, k1);
        // Increase intensity
        myservo->writeServo(sPin[k], (pos_deg +  ANGLE_INCREMENT) );
    }
}

void motorObj::cleanUpAfterSlowOpen(){
    if ( myservo->read(sPin[0])> SERVO_MIN_ANGLE){
        blindsOpen = true;   
    }
    detachAll();
}

bool motorObj::isBlindOpen(){
    return blindsOpen;
}
int motorObj::getLimitFlag(){
    return limitFlag;
}
int motorObj::getPositionOfSlider(long int Pos){
    if (Pos == -1){
        return map(getPosition(0), lowestPosition,highestPosition,sliderMin, sliderMax);  
    }
    updateTime = millis();
    return  map(Pos,lowestPosition,highestPosition, sliderMin,sliderMax);
}
// Get the position translated from slider to actual machine
int motorObj::getPositionOfMotor(int sliderPos){
  updateTime = millis(); 
  if (sliderPos==-1){
      return  getPosition(0); // get position of the motor at index0;
  }
  return map(sliderPos, sliderMin,sliderMax,minOpenAngle,maxOpenAngle);
}

bool motorObj::isOpen(){
    blindsOpen = false;
    int angle;
    for(int k=0; k<numServos;k++){
        angle = (int) getPosition(k);
        if( angle <135 && angle>45){
            Serial.printf( "window %d is open @ %d degrees \n", k, angle);
            blindsOpen = true;
        }
    }
    return blindsOpen;
}

String motorObj::getBlindName(){
    return blindName;
}

void motorObj::setBlindName(String blindname){
    blindName = blindname;
}

int motorObj::getFeedback(int Pin)
{
    double test, result, mean;
    for (int j = 0; j < 20; j++)
    {
        reading[j] = analogRead(Pin); // get raw data from servo potentiometer
        delay(100);
    }             
    // sort the readings low to high in array
    bool done = false; // clear sorting flag
    while (done != true)
    { // simple swap sorts numbers from lowest to highest
        done = true;
        for (int j = 0; j < 19; j++)
        {
            if (reading[j] > reading[(j + 1)])
            { // sorting numbers here
                test = reading[j + 1];
                reading[(j + 1)] = reading[j];
                reading[j] = test;
                done = false;
            }
        }
    }
    mean = 0;
    // discard the 6 highest and 6 lowest readings
    for (int k = 6; k < 14; k++) {
        mean += reading[k];
    }
    result = mean / 8; // average useful readings
    return (result);
}
long int motorObj::getPosition(int motor_n){
    int h = getFeedback(rPin[motor_n]);
    // Serial.printf("feedback[ %d  ]- %d\n ",motor_n, h);
    h = round(map(h, maxFdBk, minFdBk, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE));
    return h;
}
void motorObj::setOpeningAngle(){
    // get the position of each to set them;
    // usually they all should be same;            
    int angle = (int)getPosition(0);
    int maxOpenAngle = angle >SERVO_MAX_ANGLE? SERVO_MAX_ANGLE: angle;
}

void motorObj::openOrCloseBlind(){
    Serial.print(" Blinds are: ");
    if (blindsOpen) {
        // Close the blinds
        attachAll();
        delay(100);
        Serial.println("Closing..");
        for (int k = 0; k< numServos; k++) {
            myservo->writeServo(sPin[k], (uint8_t) minOpenAngle );
        }
        blindsOpen = false;
        status = "close";
        delay(300);
        detachAll();
    } else {
        attachAll();
        delay(100);
        Serial.println("Opening..");
        // Open the blinds
        for (int k = 0; k< numServos; k++) {
            myservo->writeServo(sPin[k], maxOpenAngle );
        }
        blindsOpen = true;
        status = "open";
        delay(300);
        detachAll();
    }
}

void motorObj::openBlinds(){
    attachAll();
    delay(100);
    Serial.println("Closing..");
    for (int k = 0; k< numServos; k++) {
        myservo->writeServo(sPin[k], (uint8_t) maxOpenAngle );
    }
    blindsOpen = false;
    status = "open";
    delay(300);
    detachAll();
}

void motorObj::closeBlinds(){
        // Close the blinds
    attachAll();
    delay(100);
    Serial.println("Closing..");
    for (int k = 0; k< numServos; k++) {
        myservo->writeServo(sPin[k], (uint8_t) minOpenAngle );
    }
    blindsOpen = false;
    status = "close";
    delay(300);
    detachAll();
}

long int motorObj::ifRunningHalt(){
    // literally do nothing for Servo.
    // as they are supposed to be very fast.
    return getPosition(0);
}

int motorObj::setWindowMax(int pos){
    // do nothing with the servo just return the slider value for current poisition
    return getPositionOfSlider(-1);
}
int motorObj::setWindowLow(int pos){
    // do nothing, servo are fast enough and not worth setting limits ;
    return getPositionOfSlider( (long int)  -1);
}

void motorObj::FactoryReset(){
    // YTD

    // reset everything to the orginal format including name of the blind

    // wipe away the slate clean;
}

void motorObj::setSide(int direction, int index){
    // index: is the index of the motor that being identified as left or right
    // direction : is either 1 or -1
}

// move to the given angle;
void motorObj::moveBlinds(int Pos){
    attachAll();
    delay(100);
    // Open the blinds
    for (int k = 0; k< numServos; k++) {
        myservo->writeServo(sPin[k], Pos );
    }
    blindsOpen = isBlindOpen();
    status = blindsOpen? "open":"close";
    delay(300);
    detachAll();
}