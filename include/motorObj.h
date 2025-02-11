#ifndef MOTOR_OBJ_H
#define MOTOR_OBJ_H
#include <pwmWrite.h>
#include "EEPROM.h"
#include <Preferences.h>

#define EEPROM_ADDRESS 0x00
#ifndef DEBUG
#define DEBUG 1
#endif

#define SERVO_MAX_ANGLE 180
#define SERVO_MIN_ANGLE  0

class motorObj{

    private:
        // servo parameters
        const int spinlist[4] = {10, 9, 7, 6};
        const int rpinlist[4] = {0, 1, 2, 3}; //analogpins
        int numServos;
        // servo Pins they are all standard
        int *sPin =nullptr;
        // readPins
        int *rPin =nullptr; 
        int *direction = nullptr;
        // current angle 
        int currentAngle = 0 ; // completely cosed is what you start with.

        // saving bool 
        bool savedFlag = false;

        float feedback;
        Pwm *myservo = nullptr;
        bool servoAttached=false;
        double reading[20];
        bool Initialized = false;

        Preferences MYPrefs;
        
        // semi permanant max and min angles;
        int maxOpenAngle = SERVO_MAX_ANGLE;
        int minOpenAngle = SERVO_MIN_ANGLE;

        // slider position        
        int sliderMin = 0;
        int sliderMax = 100;
        int currentSliderPosition=-1; // between 0 - 100

        bool blindsOpen=false;
        long int updateTime;
        int  limitFlag; // 0 none set, 1 lowerlimit Set, 2 upperlimit set 3 - both limits set
        String blindName="";

        // memory size = 
        const int totalEEPROMSize = 10*sizeof(int)+ 10*sizeof(char);

        void initializePins(int numMotors);
        void setDirections(int * dirs);
        void initializeServo();
        // check if the motor with spin is attached;
        bool isAttached(int Pin);

        // sticky flag for servos's get_pos values using read
        bool readFlag = false;

    public:
        String status; // indicator of "open" or "close"
        // blank construtor for reading EEPROM
        motorObj();

        motorObj(int numMotors);

        motorObj(int numMotors, int * dirs);

        ~motorObj();



        void attachAll();
        void detachAll();
        void slowMove();
        bool isOpen();
        bool isBlindOpen();
        // angle to which to open the motor to.
        void setOpeningAngle(int angle=-1);
        void setClosingAngle(int angle = -1);
        void setBlindName(const String &name);

        bool isInitialized();

        bool setLimits(int sliderpos=-1);

        void openOrCloseBlind();
        void openBlinds();
        void closeBlinds();
        int getFeedback(int);
        int getPosition(int);
        int getPosWoAttaching(int motor_n);

        void cleanUpAfterSlowMove();

        // get position of the slider converted to motor position
        int getPositionOfMotor(int sliderPos=-1);
        // get (current) position of motor translate to slider
        int getPositionOfSlider(int angle=-1);
        // return the stored values;
        int getCurrentSliderPosition();

        int getLimitFlag();

        String getBlindName();

        int getAvgFeedback(int Pin);

        int getServoCount();

        int *getDirections();
        long int ifRunningHalt();

        void FactoryReset();
        void moveBlinds(int angle);

        void saveMotorParameters();  // save parameter of the various motors to the EEPROM
        void loadMotorParameters();
        bool isEEPROMRangeEmpty();
};

#endif