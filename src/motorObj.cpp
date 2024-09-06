
#include <motorObj.h>
#define maxUs 2580 // for 180servo this is best
#define minUs  315 // min position for 180Servo;
#define ANGLE_INCREMENT 5
#define maxFdBk 4095
#define minFdBk 515



motorObj::motorObj(){

    // create sPin object

    // direction = new int[numServos];

    // initialize the sPin and rPin as per ESP32c3 super Mini by default;
    // Initialize pins
    myservo = new Pwm();
    for(int k=0; k<numServos;k++){
        pinMode(sPin[k], OUTPUT);
        pinMode(rPin[k], INPUT);
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
    // delete[] sPin;
    // delete[] rPin;
    // delete[] direction;
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
}

bool motorObj::isBlindOpen(){
    return blindsOpen;
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
long motorObj::getPosition(int motor_n){
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
        Serial.println("Closing..");
        for (int k = 0; k< numServos; k++) {
            myservo->writeServo(sPin[k], (uint8_t) minOpenAngle );
        }
        blindsOpen = false;
    } else {
        Serial.println("Opening..");
        // Open the blinds
        for (int k = 0; k< numServos; k++) {
            myservo->writeServo(sPin[k], maxOpenAngle );
        }
        blindsOpen = true;
    }
}
