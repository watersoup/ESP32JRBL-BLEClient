
#include <motorObj.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>

#define maxUs 2580 // for 180servo this is best
#define minUs 315  // min position for 180Servo;
#define ANGLE_INCREMENT 5
#define maxFdBk 4095
#define minFdBk 515
#define junkfeed 0xe000
// this constructor is to make sure that the if it was already initialized
// it will read from the EEPROM
motorObj::motorObj() : numServos(0)
{
    // check EEPROM memory
    if(DEBUG) Serial.println("Initializing EEPROM of size: " + String(totalEEPROMSize) + " bytes");

    // MYPrefs.begin("system", false);
    // MYMEM.begin(EEPROM_ADDRESS);

    delay(100); 

    if (!isEEPROMRangeEmpty())
    {
        if (DEBUG)
            Serial.println(" EEPROM is not empty ");
        loadMotorParameters();
        Serial.println(" numServos loaded:"+ String(numServos));
        initializePins(numServos);
        initializeServo();
        moveBlinds(getPositionOfMotor(currentSliderPosition));
        Initialized=true;
        //initialize the blindName if not set;
        if (blindName == "") blindName = "JRBlind";
    }
    else
    {
        Initialized=false;
        if (DEBUG)
            Serial.println("EEPROM  is Empty");
        // MYMEM is empty, initialize with default value
    }
}

motorObj::motorObj(int numMotors) : numServos(numMotors)
{
    // MYMEM.begin(EEPROM_ADDRESS);

    currentSliderPosition = sliderMin;
    // initialize the sPin and rPin as per ESP32c3 super Mini by default;
    // Initialize pins
    initializePins(numServos);
    initializeServo();

}

motorObj::motorObj(int numMotors, int *dirs) : numServos(numMotors)
{
    // MYMEM.begin(EEPROM_ADDRESS);

    currentSliderPosition = sliderMin;

    // initialize the sPin and rPin as per ESP32c3 super Mini by default;
    
    // Initialize pins

    initializePins(numServos);

    setDirections(dirs);

    initializeServo();

}

motorObj::~motorObj()
{
    // destroy sPin and rPin;
    // destroy directions created;
    if ( sPin !=nullptr){
        delete[] sPin;
        sPin = nullptr;
    }
    if ( rPin !=nullptr){
        delete[] rPin;
        rPin = nullptr;
    }

    if ( direction != nullptr){
        delete[] direction;
        direction = nullptr;
    }

    if ( myservo != nullptr){
        delete myservo;
        myservo = nullptr;
    }
}

bool motorObj::isInitialized()
{
    return Initialized;
}

void motorObj::initializeServo()
{
    int adjustedAngle  = (SERVO_MIN_ANGLE+SERVO_MAX_ANGLE)/2;
    if(DEBUG) Serial.print("InitializeServo..");
    myservo = new Pwm();
    for (int k = 0; k < numServos; k++)
    {
        pinMode(sPin[k], OUTPUT);
        pinMode(rPin[k], INPUT_PULLDOWN);
        // if (direction[k] == 1) myservo->attachServo(sPin[k], minUs, maxUs);
        // else myservo->attachServo(sPin[k],  maxUs, minUs);
        myservo->attachServo(sPin[k], minUs, maxUs);
        delay(500);
        // adjustedAngle = direction[k] == 1 ?   SERVO_MAX_ANGLE: SERVO_MIN_ANGLE;


        myservo->writeServo(sPin[k], adjustedAngle ); // start from closed position

        delay(1000);
        myservo->detach(sPin[k]);
        
        if(DEBUG) Serial.printf("\tNservo=%d [%d] -> %d", numServos,k,adjustedAngle);
        blindsOpen = false;
    }
    if(DEBUG) Serial.println(" Done");
}

void motorObj::setBlindName(const String& name)
{
    blindName = name;
}

void motorObj::initializePins(int numMotors)
{
    if(DEBUG) Serial.print("InitializePins..");
    // create sPin object
    sPin = new int[numServos];
    rPin = new int[numServos];
    for (int k = 0; k < numServos; k++)
    {
        Serial.printf("\tk=%d",k);
        sPin[k] = spinlist[k];
        rPin[k] = rpinlist[k];
    }
    if(DEBUG) Serial.println(".. Done");

}

void motorObj::setDirections(int *dirs)
{
    // Check if the input pointer is null
    if (dirs == nullptr) {
        Serial.println("ERROR: Invalid input array (null pointer).");
        return;
    }

    // Allocate memory for direction if not already done, or reallocate if needed
    if (direction != nullptr) {
        delete[] direction; // Free previously allocated memory
    }

    direction = new int[numServos]; // Allocate new memory for the directions
    Serial.print("Directions..");
    // Copy directions
    for (int k = 0; k < numServos; k++) {
        direction[k] = dirs[k];
        Serial.printf(" %d ", direction[k]);
    }
    if(DEBUG) Serial.println(" ... set successfully.");
}

void motorObj::attachAll()
{
    int k1;
    if (!servoAttached)
    {
        if(DEBUG) Serial.println("Attaching all servos");

        for (int k = 0; k < numServos; k++)
        {
            // if (direction[k] == 1) k1 = myservo->attachServo(sPin[k], minUs, maxUs);
            k1 = myservo->attachServo(sPin[k], minUs, maxUs);
            // else k1 = myservo->attachServo(sPin[k],  maxUs,minUs);
            if (k1 == 253 || k1 == 255)
            {
                Serial.printf(" Couldn't Attach : %d \n", k1);
                delay(5000);
            }
        }
    }
    readFlag = true;
    servoAttached = true;
}

void motorObj::detachAll()
{
    currentSliderPosition = getPositionOfSlider(-1);
    if (servoAttached)
    {
        for (int k = 0; k < numServos; k++)
        {
            myservo->detach(sPin[k]);
        }
    }
    if(DEBUG) Serial.println("Detached all servos");
    servoAttached = false;
}

void motorObj::slowMove()
{
    int ch, pos_deg, newAngle;
    // Button pressed, handle the press
    for (int k = 0; k < numServos; k++)
    {
        pos_deg=-1;
        if (readFlag ) pos_deg = getPosWoAttaching(k);
        else pos_deg = myservo->read(sPin[k]);
        pos_deg = constrain(pos_deg, 0, 180);        
        if (!blindsOpen)
            newAngle =  (pos_deg + direction[k]*ANGLE_INCREMENT);
        else
            newAngle =  (pos_deg - direction[k]*ANGLE_INCREMENT);

        if(DEBUG) Serial.printf(".. dir = %d, posdeg : %d -> %d\n",direction[k], pos_deg,  newAngle);
        // Increase intensity
        myservo->writeServo(sPin[k], newAngle);
        delay(10);
    }
    readFlag = false;
}

void motorObj::cleanUpAfterSlowMove()
{
    blindsOpen = !blindsOpen;
    detachAll();
    saveMotorParameters();
}

bool motorObj::isBlindOpen()
{
    return blindsOpen;
}

int motorObj::getLimitFlag()
{
    return limitFlag;
}

int motorObj::getCurrentSliderPosition(){
    return currentSliderPosition;
}

// get position translate from machine to slider position;
// it also gets current position translated;
int motorObj::getPositionOfSlider(int angle)
{
    
    if (angle == -1)
    {
        if(DEBUG) Serial.println("getPositionOfSlider : "+ String(angle));
        return map( getPosition(0), minOpenAngle, maxOpenAngle, sliderMin, sliderMax);
        
    }
    updateTime = millis();

    return map( angle ,minOpenAngle, maxOpenAngle,  sliderMin, sliderMax);
}

// Get the position translated from slider to actual machine
// or just the current angle of machine;
int motorObj::getPositionOfMotor(int sliderPos)
{
    updateTime = millis();
    if (sliderPos == -1)
    {
        return getPosition(0); // get position of the motor at index0;
    }
    if(DEBUG) Serial.printf("sliderPos : %d\n", sliderPos );

    return map(sliderPos, sliderMin, sliderMax, minOpenAngle, maxOpenAngle);
}

bool motorObj::isOpen()
{
    blindsOpen = false;
    int angle;
    for (int k = 0; k < numServos; k++)
    {
        angle = (int)getPosition(k);
        if (angle < 135 && angle > 45)
        {
            Serial.printf("window %d is open @ %d degrees \n", k, angle);
            blindsOpen = true;
        }
    }
    return blindsOpen;
}

int motorObj::getServoCount()
{
    return numServos;
}

int * motorObj::getDirections(){
    int * dir = new int[numServos];
    for ( int k =0;k<numServos;k++){
        dir[k] = direction[k];
    }
    
    return dir;
}

String motorObj::getBlindName()
{
    return blindName;
}

int motorObj::getAvgFeedback(int Pin)
{
    double test, result, mean;
    if(DEBUG) Serial.println("getFdBk: Pin = " + String(Pin));

    for (int j = 0; j < 20; j++)
    {
        reading[j] = analogRead((uint8_t) Pin); // get raw data from servo potentiometer
        delay(100);
    }
    if(DEBUG) Serial.println("getFdBk: Pin = " + String(Pin));
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
    Serial.println("fdBk: done while..");

    mean = 0;
    // discard the 6 highest and 6 lowest readings
    for (int k = 6; k < 14; k++)
    {
        mean += reading[k];
    }
    result = mean / 8; // average useful readings
    Serial.println("fdBk: " + String(result));
    return (int) result;
}

// get quick analog feedback

int motorObj::getFeedback(int Pin)
{
    double test, result, mean;
    // if(DEBUG) Serial.println("getFdBk: Pin = " + String(Pin));
    mean = 0;
    for (int j = 0; j < 10; j++)
    {
        mean += analogRead((uint8_t) Pin); // get raw data from servo potentiometer
    }
    result = constrain(mean / 10,  minFdBk,maxFdBk ); // average useful readings
    Serial.println("fdBk: " + String(result));
    return (result);
}


// get position of the motor without attaching useful 
// in rare scenario;
long int motorObj::getPosWoAttaching(int motor_n)
{
    //make use of feedback function from 
    int fdbk = getFeedback(rPin[motor_n]);
    return round(map( fdbk, maxFdBk, minFdBk, SERVO_MIN_ANGLE,SERVO_MAX_ANGLE));
}

bool motorObj::isAttached(int Pin)
{
    int ch = myservo->attached(Pin);
    return ((ch != 255) && (ch != 253));
}

long int motorObj::getPosition(int motor_n)
{
    bool attachedHere=false;
    if (!isAttached(sPin[motor_n])){
        myservo->attachServo(sPin[motor_n],  minUs , maxUs);
        delay(100);
        attachedHere=true;
    }

    int h = myservo->read(sPin[motor_n]) ;
    if ( direction [motor_n] == -1) h = SERVO_MAX_ANGLE - h + SERVO_MIN_ANGLE;

    Serial.printf(" angle[ %d  ]- %d\n ",motor_n, h);

    // h = round(map(h, maxFdBk, minFdBk, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE));
    // if you attached it separately then detach it
    if (attachedHere) {
        myservo->detach(sPin[motor_n]);
        delay(100);
    }

    return h;
}

void motorObj::setOpeningAngle(int angle)
{
    // get the position of each to set them;
    // usually they all should be same;
    if (angle == -1)
    {
        angle = getPosition(numServos-1); // get position of index motor
        
        if (direction[numServos-1] == -1) angle = SERVO_MAX_ANGLE - angle + SERVO_MIN_ANGLE;
        Serial.printf("Setting Max Opening Angle: %d\n", angle);
    }
    delay(100);
    
    maxOpenAngle = angle > SERVO_MAX_ANGLE ? SERVO_MAX_ANGLE : angle;
    // else 
        // minOpenAngle = angle < SERVO_MIN_ANGLE?  SERVO_MIN_ANGLE : angle;
    if (limitFlag!=3 && limitFlag!=2) limitFlag +=2;
}

void motorObj::setClosingAngle(int angle)
{
    // get the position of each to set them;
    // usually they all should be same;
    if (angle == -1)
    {
        angle = getPosition(numServos-1); // get position of index motor
        if (direction[numServos-1]==-1) angle = SERVO_MAX_ANGLE - angle + SERVO_MIN_ANGLE;
        Serial.printf("Setting Closing Angle: %d\n", angle);
    }
    minOpenAngle = (angle < SERVO_MIN_ANGLE ? SERVO_MIN_ANGLE : angle);
    if (limitFlag!=3 && limitFlag!=1) limitFlag +=1;
}

void motorObj::openOrCloseBlind()
{
    Serial.print(" Blinds are: ");
    if (blindsOpen)
    {
        // Close the blinds
        closeBlinds();
    }
    else
    {
        // Open the blinds
        openBlinds();
    }
    saveMotorParameters();

}

void motorObj::openBlinds()
{
    int adjustedAngle;
    attachAll();
    delay(100);
    if(DEBUG) Serial.printf("Opening.. to %d", maxOpenAngle);
    for (int k = 0; k < numServos; k++)
    {
        adjustedAngle = (direction[k] == 1 ? maxOpenAngle : minOpenAngle);
        // if(DEBUG) Serial.printf("\t [%d] to %d", k+1, adjustedAngle);
        myservo->writeServo(sPin[k], adjustedAngle);
        // myservo->writeServo(sPin[k], maxOpenAngle);

        delay(500);
    }
    if (DEBUG) Serial.println("..Done");
    blindsOpen = true;
    status = "open";
    delay(300);
    detachAll();
}

void motorObj::closeBlinds()
{
    // Close the blinds
    int adjustedAngle ;
    attachAll();
    delay(100);
    if(DEBUG) Serial.printf("Closing.. to %d", minOpenAngle);
    for (int k = 0; k < numServos; k++)
    {
        adjustedAngle = direction[k] == 1 ? minOpenAngle : maxOpenAngle;
        // if(DEBUG) Serial.printf("\t [%d] to %d", k+1, adjustedAngle);
        myservo->writeServo(sPin[k], adjustedAngle);
        // myservo->writeServo(sPin[k], minOpenAngle);
        delay(500);
    }
    blindsOpen = false;
    status = "close";
    if (DEBUG) Serial.println("..Done");
    delay(300);
    detachAll();
}

long int motorObj::ifRunningHalt()
{
    // literally do nothing for Servo.
    // as they are supposed to be very fast.
    return getPosition(0);
}

void motorObj::FactoryReset()
{
    if (DEBUG)
        Serial.println("Resetting  blindsObj" + blindName);
    blindName = "";
    maxOpenAngle = SERVO_MAX_ANGLE;
    minOpenAngle = SERVO_MIN_ANGLE;
    delete[] direction;
    limitFlag = 0;
    blindsOpen = false;
    status = "close";
    // reset everything to the orginal format including name of the blind
    if (DEBUG)
        Serial.print(" Emptying the memory");
    // for (int i = EEPROM_ADDRESS; i < (EEPROM_ADDRESS+ totalEEPROMSize); i++)
    // {
    //     MYMEM.writeByte(i, 0xFF);
    // }
    // MYMEM.commit();
    MYPrefs.begin("system", false);
    delay(100);
    MYPrefs.clear();
    MYPrefs.end();
    delay(100);

    if (DEBUG)
        Serial.println(" ...Done");

    // wipe away the slate clean;
}


// move to the given angle;
void motorObj::moveBlinds(int angle)
{
    int adjustedAngle;
    attachAll();
    delay(100);
    // Move each servo to the given angle, taking direction into account
    for (int k = 0; k < numServos; k++)
    {

        adjustedAngle = direction[k] == 1 ? angle : (maxOpenAngle - angle + minOpenAngle);
        
        myservo->writeServo(sPin[k], angle);
        delay(50);
    }
    blindsOpen = isBlindOpen();
    status = blindsOpen ? "open" : "close";
    delay(300);
    detachAll();
    delay(100);
    saveMotorParameters();
}

// save motor parameters to MYMEM
void motorObj::saveMotorParameters()
{

    size_t bytesWritten;
    if(DEBUG) Serial.print("Saving motor ");

    MYPrefs.begin("system", false);
    delay(100);
    MYPrefs.putInt("numServos", numServos);
    bytesWritten = MYPrefs.putBytes("dirs", (byte *) direction, numServos*sizeof(int));
    if (DEBUG && bytesWritten == sizeof(direction)) Serial.println("Directions saved.");
    delay(40);
    MYPrefs.putBool("BldOflag", blindsOpen);
    delay(10);
    MYPrefs.putInt("curSlPos", currentSliderPosition);
    delay(10);
    MYPrefs.putInt("limitFlag", limitFlag);
    delay(10);
    MYPrefs.putInt("maxOangle", maxOpenAngle);
    delay(10);
    MYPrefs.putInt("minOangle", minOpenAngle);
    delay(10);
    MYPrefs.putString("blindName", blindName.substring(0,10));
    delay(100);
    MYPrefs.end();

  
    // if(DEBUG) Serial.print(" .");
    // MYMEM.commit(); // Don't forget to commit changes
    // // esp_task_wdt_reset();  // Feed the watchdog
    if (DEBUG)
        Serial.println("SAVED Parameters for blinds : " + blindName);
}

bool motorObj::isEEPROMRangeEmpty()
{
    bool isEmpty = true;
    MYPrefs.begin("system", false);
    delay(100);
    isEmpty = (MYPrefs.getInt("numServos", -1) == -1);
    delay(100);
    MYPrefs.end();
    return isEmpty;
}

// save motor parameters to MYMEM
void motorObj::loadMotorParameters()
{
    if(DEBUG) Serial.println("LoadMotorParameters...");
    // int address = EEPROM_ADDRESS;
    MYPrefs.begin("system", false);
    delay(100);

    numServos = MYPrefs.getInt("numServos", numServos);
    if(DEBUG) Serial.println("numServos: "+String(numServos));

    numServos = (numServos>4?4:numServos);

    if (direction == nullptr){
        direction = new int[numServos];
    }
    MYPrefs.getBytes("dirs", (byte *) direction, numServos*sizeof(int));
    delay(100);
    // for(int k=0;k<numServos;k++){
    //     if(DEBUG) Serial.printf("Direction [ %d ] = %d\n", k, direction[k]);
    // }

    blindsOpen = MYPrefs.getBool("BldOflag", false);

    currentSliderPosition = MYPrefs.getInt("curSlPos", -1);

    limitFlag = MYPrefs.getInt("limitFlag", 0);

    maxOpenAngle = MYPrefs.getInt("maxOangle", SERVO_MAX_ANGLE);

    minOpenAngle = MYPrefs.getInt("minOangle", SERVO_MIN_ANGLE);

    blindName = MYPrefs.getString("blindName", "");



    MYPrefs.end();
    delay(200);

    if (DEBUG)
        Serial.printf("parameter Loaded: %s \n\t #servo: %d \n\tOpenFlag: %d\n\
        \tCurrentSliderPos: %d\n\tLimitFlag: %d\n\tMaxOpenAngle: %d\n\tMinOpenAngle: %d\n", \
            blindName.c_str(),numServos, blindsOpen , currentSliderPosition, limitFlag, \
            maxOpenAngle, minOpenAngle);

}