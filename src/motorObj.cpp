
#include <motorObj.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>

#define maxUs 2580 // for 180servo this is best
#define minUs 315  // min position for 180Servo;
#define ANGLE_INCREMENT 5
#define maxFdBk 4095
#define minFdBk 515


// this constructor is to make sure that the if it was already initialized
// it will read from the EEPROM
motorObj::motorObj() : numServos(0)
{
    // check EEPROM memory
    if(DEBUG) Serial.println("Initializing EEPROM of size: " + String(totalEEPROMSize) + " bytes");

    EEPROM.begin(totalEEPROMSize + EEPROM_ADDRESS);
    delay(100);

    if (!isEEPROMRangeEmpty(EEPROM_ADDRESS, 0x080 ))
    {
        if (DEBUG)
            Serial.println(" EEPROM is not empty ");
        loadMotorParameters();
        Serial.println(" numServos loaded:"+ String(numServos));
        initializePins(numServos);
        initializeServo();

    }
    else
    {
        if (DEBUG)
            Serial.println("EEPROM  is Empty");
        // EEPROM is empty, initialize with default value
    }
}

motorObj::motorObj(int numMotors) : numServos(numMotors)
{

    currentSliderPosition = sliderMin;
    // initialize the sPin and rPin as per ESP32c3 super Mini by default;
    // Initialize pins
    initializePins(numServos);
    initializeServo();

}

motorObj::motorObj(int numMotors, int *dirs) : numServos(numMotors)
{

    currentSliderPosition = sliderMin;

    // initialize the sPin and rPin as per ESP32c3 super Mini by default;
    
    // Initialize pins

    initializePins(numServos);

    setDirections(dirs);

    initializeServo();

    // initialize EEPROM
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

void motorObj::initializeServo()
{
    int adjustedAngle ;
    if(DEBUG) Serial.print("InitializeServo..");
    myservo = new Pwm();
    for (int k = 0; k < numServos; k++)
    {
        pinMode(sPin[k], OUTPUT);
        pinMode(rPin[k], INPUT_PULLDOWN);
        myservo->attachServo(sPin[k], minUs, maxUs);
        delay(500);
        adjustedAngle = direction[k] == 1 ?   SERVO_MAX_ANGLE: SERVO_MIN_ANGLE;
        myservo->writeServo(sPin[k], adjustedAngle);

        delay(1000);
        myservo->detach(sPin[k]);
        
        if(DEBUG) Serial.printf("Initiate Servos: numservos: %d, K = %d\n", numServos,k);
    }
    if(DEBUG) Serial.println(" Done");
}

void motorObj::setBlindName(String name)
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
        Serial.printf("\tk=%d\n",k);
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

    // Copy directions
    for (int k = 0; k < numServos; k++) {
        direction[k] = dirs[k];
    }

    if(DEBUG) Serial.println("Directions set successfully.");
}

void motorObj::attachAll()
{
    int k1;
    if (!servoAttached)
    {
        for (int k = 0; k < numServos; k++)
        {
            k1 = myservo->attachServo(sPin[k], minUs, maxUs);
            if (k1 == 253 || k1 == 255)
            {
                Serial.printf(" Couldn't Attach : %d \n", k1);
                delay(5000);
            }
        }
    }
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
    servoAttached = false;
}

void motorObj::slowOpen()
{
    int ch, pos_deg, adjustedAngle;
    // Button pressed, handle the press
    for (int k = 0; k < numServos; k++)
    {
        pos_deg=-1;
        ch = myservo->attached(sPin[k]);
        pos_deg = myservo->read(sPin[k]);
        Serial.printf(".. posdeg : %d ch: %d \n", pos_deg, ch);
        adjustedAngle =  direction[k]*ANGLE_INCREMENT;
        // Increase intensity
        myservo->writeServo(sPin[k], (pos_deg + adjustedAngle));
        delay(20);
    }
}

void motorObj::cleanUpAfterSlowOpen()
{
    blindsOpen = ! blindsOpen;
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

int motorObj::getFeedback(int Pin)
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
    return (result);
}

// get position of the motor without attaching useful 
// in rare scenario;
long int motorObj::getPosWoAttaching(int motor_n)
{
    //make use of feedback function from 
    int fdbk = getFeedback(motor_n);
    return round(map( fdbk, maxFdBk, minFdBk, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE));
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
        myservo->attachServo(sPin[motor_n], minUs, maxUs);
        delay(100);
        attachedHere=true;
    }

    int h = myservo->read(sPin[motor_n]) ;
    Serial.printf("ch = %d ; angle[ %d  ]- %d\n ",motor_n, h);

    // adjusted angle based on direction;
    h = (direction[motor_n] == 1? h : maxOpenAngle - h );

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
        Serial.printf("Setting Max Open Angle: %d\n", angle);
    }
    maxOpenAngle = angle > SERVO_MAX_ANGLE ? SERVO_MAX_ANGLE : angle;
    if (limitFlag!=3 && limitFlag!=2) limitFlag +=2;
}

void motorObj::setClosingAngle(int angle)
{
    // get the position of each to set them;
    // usually they all should be same;
    if (angle == -1)
    {
        angle = getPosition(numServos-1); // get position of index motor
        Serial.printf("Setting Max Open Angle: %d\n", angle);
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
    attachAll();
    delay(100);
    Serial.println("Opening..");
    for (int k = 0; k < numServos; k++)
    {
        int adjustedAngle = (direction[k] == 1 ? maxOpenAngle : minOpenAngle);
        myservo->writeServo(sPin[k], adjustedAngle);
        delay(1000);
    }
    blindsOpen = true;
    status = "open";
    delay(300);
    detachAll();
}

void motorObj::closeBlinds()
{
    // Close the blinds
    attachAll();
    delay(100);
    Serial.println("Closing..");
    for (int k = 0; k < numServos; k++)
    {
        int adjustedAngle = direction[k] == 1 ? minOpenAngle : maxOpenAngle;
        myservo->writeServo(sPin[k], adjustedAngle);
        delay(1000);
    }
    blindsOpen = false;
    status = "close";
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
    for (int i = EEPROM_ADDRESS; i < totalEEPROMSize; i++)
    {
        EEPROM.put(i, 0xFF);
    }
    EEPROM.commit();
    delay(1000);
    if (DEBUG)
        Serial.println(" ...Done");

    // wipe away the slate clean;
}


// move to the given angle;
void motorObj::moveBlinds(int angle)
{
    attachAll();
    delay(100);
    // Move each servo to the given angle, taking direction into account
    for (int k = 0; k < numServos; k++)
    {
        int adjustedAngle = direction[k] == 1 ? angle : (maxOpenAngle - angle);
        myservo->writeServo(sPin[k], adjustedAngle);
        delay(50);
    }
    blindsOpen = isBlindOpen();
    status = blindsOpen ? "open" : "close";
    delay(300);
    detachAll();
    delay(100);
    saveMotorParameters();
}

// save motor parameters to EEPROM
void motorObj::saveMotorParameters()
{

    if(DEBUG) Serial.print("Saving motor ");
    int address = EEPROM_ADDRESS;
    int tmpVal;

    tmpVal = numServos;
    EEPROM.put(address, tmpVal);
    address += sizeof(int);

    if(DEBUG) Serial.print(" .");
    for (int k = 1; k <= 4; k++)
    {
        if (k <= numServos)
            EEPROM.put(address, direction[k - 1]);
        address += sizeof(int);
        // esp_task_wdt_reset();  // Feed the watchdog

    }
    if(DEBUG) Serial.print(" .");
    int blindOpenInt = (blindsOpen ? 1 : 0);
    EEPROM.put(address, blindOpenInt);
    address += sizeof(int);

    if(DEBUG) Serial.print(" .");
    // Save integer parameters
    EEPROM.put(EEPROM_ADDRESS, currentSliderPosition);
    address += sizeof(int);

    if(DEBUG) Serial.print(" .");
    EEPROM.put(address, limitFlag);
    address += sizeof(int);

    if(DEBUG) Serial.print(" .");
    EEPROM.put(address, maxOpenAngle);
    address += sizeof(int);
    
    if(DEBUG) Serial.print(" .");
    EEPROM.put(address, minOpenAngle);
    address += sizeof(int);

    // Save String parameter
    if(DEBUG) Serial.print(" .");
    // Convert the String to a char array
    char blindNameArray[10];  // Assuming a max name length of 32 characters
    blindName.toCharArray(blindNameArray, sizeof(blindNameArray));    
    EEPROM.writeString(address, blindNameArray);
    address +=sizeof(blindNameArray);
    
    if(DEBUG) Serial.print(" .");
    EEPROM.commit(); // Don't forget to commit changes
    // esp_task_wdt_reset();  // Feed the watchdog
    if (DEBUG)
        Serial.println("SAVED Parameters for blinds : " + blindName);
    delay(100);
}

bool motorObj::isEEPROMRangeEmpty(int startAddress, int endAddress)
{
    for (int i = startAddress; i <= endAddress; i++)
    {
        if (EEPROM.read(i) != 0xFF)
        {
            return false;
        }
    }
    return true;
}

// save motor parameters to EEPROM
void motorObj::loadMotorParameters()
{

    if(DEBUG) Serial.println("LoadMotorParameters...");
    int address = EEPROM_ADDRESS;
    // Save integer parameters
    int N;
    EEPROM.get(address,N);
    address += sizeof(int);
    if (DEBUG) Serial.println("numServo: " + String(N));
    numServos = N;

    int dir;
    for (int k = 0; k < 4; k++)
    {
        if (k < numServos){
            EEPROM.get(address, dir);
            if (DEBUG) Serial.printf("Direction [ %d ] = %d\n", k, dir);
            direction[k] = dir;
        } 
        address += sizeof(int);
        
    }

    int blindOpenInt;
    EEPROM.get(address, blindOpenInt);
    blindsOpen = (blindOpenInt == 1 ? true : false);
    address += sizeof(int);

    EEPROM.get(address, currentSliderPosition);
    address += sizeof(int);

    EEPROM.get(address, limitFlag);
    address += sizeof(int);

    EEPROM.get(address, maxOpenAngle);
    address += sizeof(int);
    if (DEBUG) Serial.printf("maxOpenAngle: %d\n", maxOpenAngle);

    EEPROM.get(address, minOpenAngle);
    address += sizeof(int);
    if (DEBUG) Serial.printf("minOpenAngle: %d\n", minOpenAngle);

    // Retrieve the blind name as a char array
    char blindNameArray[10];  // Assuming a max name length of 32 characters
    EEPROM.get(address, blindNameArray);
    blindName = String(blindNameArray);  // Convert char array back to String
    address += sizeof(blindNameArray);

    if (DEBUG)
        Serial.printf("parameter Loaded: %s \n\t #servo: %d \n\tOpenFlag: %d\n", 
            blindName.c_str(),numServos, blindOpenInt );
    delay(200);
    if (blindName != "" )
    {
        if ( currentSliderPosition!=-1 )
            moveBlinds(getPositionOfMotor(currentSliderPosition));
        return;
    }
    if(DEBUG) Serial.println("ERROR blindName is missing.");
}