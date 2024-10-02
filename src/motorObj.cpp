
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
    if(DEBUG) Serial.println("Initializing EEPROM of size:" + String(totalEEPROMSize));

    EEPROM.begin(totalEEPROMSize);
    delay(100);

    if (!isEEPROMRangeEmpty(EEPROM_ADDRESS, 4))
    {
        if (DEBUG)
            Serial.println(" EEPROM is not empty ");
        loadMotorParameters();
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
        delete[] rPin;
        sPin = nullptr;
        rPin = nullptr;
    }

    if ( direction != nullptr){
        delete[] direction;
        direction = nullptr;
    }

    if ( myservo != nullptr)  delete myservo;
}

void motorObj::initializeServo()
{
    if(DEBUG) Serial.print("InitializeServo..");
    myservo = new Pwm();
    for (int k = 0; k < numServos; k++)
    {
        pinMode(sPin[k], OUTPUT);
        pinMode(rPin[k], INPUT_PULLDOWN);
        myservo->attachServo(sPin[k], minUs, maxUs);
        delay(500);
        myservo->writeServo(sPin[k], SERVO_MAX_ANGLE);
        delay(1000);
        myservo->detach(sPin[k]);
        // direction[k] = 1; // set all on the same side;
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
    int k1, pos_deg;
    // Button pressed, handle the press
    for (int k = 0; k < numServos; k++)
    {
        k1 = myservo->attached(sPin[k]);
        pos_deg = myservo->read(sPin[k]);
        Serial.printf(".. posdeg : %d ch: %d \n", pos_deg, k1);
        // Increase intensity
        myservo->writeServo(sPin[k], (pos_deg + ANGLE_INCREMENT));
    }
}

void motorObj::cleanUpAfterSlowOpen()
{
    if (myservo->read(sPin[0]) > SERVO_MIN_ANGLE)
    {
        blindsOpen = true;
    }
    detachAll();
}

bool motorObj::isBlindOpen()
{
    return blindsOpen;
}
int motorObj::getLimitFlag()
{
    return limitFlag;
}
// get position translate from machine to slider position;
// it also gets current position translated;
int motorObj::getPositionOfSlider(int angle)
{
    if (angle == -1)
    {
        if(DEBUG) Serial.println("getPositionOfSlider : "+ String(angle));
        return map( getPosition(0), (long int) lowestPosition,(long int) highestPosition, (long int) sliderMin,(long int) sliderMax);
    }
    updateTime = millis();
    return map( (long int)angle,  (long int) lowestPosition,  (long int)highestPosition,  (long int) sliderMin, (long int) sliderMax);
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
    return map(sliderPos, sliderMin, sliderMax, lowestPosition, highestPosition);
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
long int motorObj::getPosition(int motor_n)
{
    int h = getFeedback(rPin[motor_n]);
    Serial.printf("feedback[ %d  ]- %d\n ",motor_n, h);
    h = round(map(h, maxFdBk, minFdBk, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE));
    return h;
}
void motorObj::setOpeningAngle(int angle)
{
    // get the position of each to set them;
    // usually they all should be same;
    if (angle == -1)
    {
        angle = getPosition(0); // get position of index motor
        Serial.printf("Setting Max Open Angle: %d\n", angle);
    }
    maxOpenAngle = angle > SERVO_MAX_ANGLE ? SERVO_MAX_ANGLE : angle;
}

void motorObj::openOrCloseBlind()
{
    Serial.print(" Blinds are: ");
    if (blindsOpen)
    {
        // Close the blinds
        attachAll();
        delay(100);
        Serial.println("Closing..");
        for (int k = 0; k < numServos; k++)
        {
            int adjustedAngle = direction[k] == 1 ? minOpenAngle : (highestPosition - minOpenAngle);
            myservo->writeServo(sPin[k], adjustedAngle);
        }
        blindsOpen = false;
        status = "close";
        delay(300);
        detachAll();
    }
    else
    {
        // Open the blinds
        attachAll();
        delay(100);
        Serial.println("Opening..");
        for (int k = 0; k < numServos; k++)
        {
            int adjustedAngle = direction[k] == 1 ? maxOpenAngle : (highestPosition - maxOpenAngle);
            myservo->writeServo(sPin[k], adjustedAngle);
        }
        blindsOpen = true;
        status = "open";
        delay(300);
        detachAll();
    }
}

void motorObj::openBlinds()
{
    attachAll();
    delay(100);
    Serial.println("Opening..");
    for (int k = 0; k < numServos; k++)
    {
        int adjustedAngle = direction[k] == 1 ? maxOpenAngle : (highestPosition - maxOpenAngle);
        myservo->writeServo(sPin[k], adjustedAngle);
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
        int adjustedAngle = direction[k] == 1 ? minOpenAngle : (highestPosition - minOpenAngle);
        myservo->writeServo(sPin[k], adjustedAngle);
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

int motorObj::setWindowMax(int angle)
{
    // do nothing with the servo just return the slider value for current poisition
    highestPosition = angle;
    maxOpenAngle = angle;
    limitFlag = limitFlag + 2;
    currentSliderPosition = getPositionOfSlider(highestPosition);
    saveMotorParameters();
    delay(100);
    return currentSliderPosition;
}
int motorObj::setWindowLow(int angle)
{
    lowestPosition = angle;
    minOpenAngle = lowestPosition;
    limitFlag = limitFlag + 1;
    currentSliderPosition = getPositionOfSlider(lowestPosition);
    saveMotorParameters();
    delay(100);
    return currentSliderPosition;
}

void motorObj::FactoryReset()
{
    if (DEBUG)
        Serial.println("Resetting  blindsObj" + blindName);
    blindName = "";
    highestPosition = SERVO_MAX_ANGLE;
    lowestPosition = SERVO_MIN_ANGLE;
    maxOpenAngle = highestPosition;
    minOpenAngle = lowestPosition;
    delete[] direction;
    limitFlag = 0;
    blindsOpen = false;
    status = "close";
    // reset everything to the orginal format including name of the blind
    if (DEBUG)
        Serial.println(" Emptying the memory");
    for (int i = EEPROM_ADDRESS; i <= totalEEPROMSize; i++)
    {
        EEPROM.put(i, 0xFF);
    }
    EEPROM.commit();
    delay(1000);
    // wipe away the slate clean;
}

void motorObj::setSide(int direction, int index)
{
    // index: is the index of the motor that being identified as left or right
    // direction : is either 1 or -1
}

// move to the given angle;
void motorObj::moveBlinds(int angle)
{
    attachAll();
    delay(100);
    // Move each servo to the given angle, taking direction into account
    for (int k = 0; k < numServos; k++)
    {
        int adjustedAngle = direction[k] == 1 ? angle : (highestPosition - angle);
        myservo->writeServo(sPin[k], adjustedAngle);
    }
    blindsOpen = isBlindOpen();
    status = blindsOpen ? "open" : "close";
    delay(300);
    detachAll();
}

// save motor parameters to EEPROM
void motorObj::saveMotorParameters()
{
    if(DEBUG) Serial.print("Saving motor ");
    int address = EEPROM_ADDRESS;

    EEPROM.put(address, numServos);
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
    EEPROM.put(address, highestPosition);
    address += sizeof(int);
    
    if(DEBUG) Serial.print(" .");
    EEPROM.put(address, lowestPosition);
    address += sizeof(int);

    // Save String parameter
    if(DEBUG) Serial.print(" .");
    EEPROM.put(address, blindName);
    esp_task_wdt_reset();  // Feed the watchdog
    
    if(DEBUG) Serial.print(" .");
    EEPROM.commit(); // Don't forget to commit changes
    // esp_task_wdt_reset();  // Feed the watchdog
    if (DEBUG)
        Serial.println("done for blinds : " + blindName);
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
    EEPROM.get(address, numServos);
    address += sizeof(int);

    for (int k = 1; k <= 4; k++)
    {
        if (k <= numServos){
            EEPROM.get(address, direction[k - 1]);
        } 
        address += sizeof(int);
    }

    int blindOpenInt;
    EEPROM.get(address, blindOpenInt);
    address += sizeof(int);

    EEPROM.get(address, currentSliderPosition);
    address += sizeof(int);

    EEPROM.get(address, limitFlag);
    address += sizeof(int);

    EEPROM.get(address, highestPosition);
    address += sizeof(int);

    EEPROM.get(address, lowestPosition);
    address += sizeof(int);

    EEPROM.get(address, blindName);

    if (DEBUG)
        Serial.println("Loaded the parameter of motor for blinds : " + blindName);
    delay(1000);
    if (blindName != "")
    {
        blindsOpen = (blindOpenInt == 1 ? true : false);
        maxOpenAngle = highestPosition;
        minOpenAngle = lowestPosition;
        moveBlinds(getPositionOfMotor(currentSliderPosition));
    }
}