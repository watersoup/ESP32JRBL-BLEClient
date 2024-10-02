#ifndef MOTOR_OBJ_H
#define MOTOR_OBJ_H
#include <pwmWrite.h>
#define EEPROM_ADDRESS 0
#ifndef DEBUG
#define DEBUG 1
#endif

#define SERVO_MAX_ANGLE 180
#define SERVO_MIN_ANGLE  0

class motorObj{

    private:
        const int spinlist[4] = {9, 8, 7, 6};
        const int rpinlist[4] = {0, 1, 2, 3}; //analogpins
        const int numServos;
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
        
        // kind of semi permanent angles;
        int highestPosition = SERVO_MAX_ANGLE;
        int lowestPosition = SERVO_MIN_ANGLE;
        
        // temporary max and min angles;
        int maxOpenAngle = SERVO_MAX_ANGLE;
        int minOpenAngle = SERVO_MIN_ANGLE;

        // slider position
        int currentSliderPosition;
        int sliderMin = 0;
        int sliderMax = 100;

        bool blindsOpen=false;
        long int updateTime;
        int  limitFlag; // 0 none set, 1 lowerlimit Set, 2 upperlimit set 3 - both limits set
        String blindName="";

        // memory size 
        const int totalEEPROMSize = 10*sizeof(int)+ 10*sizeof(char);

        void initializePins(int numMotors);
        void setDirections(int * dirs);
        void initializeServo();

    public:
        String status; // indicator of "open" or "close"
        // blank construtor for reading EEPROM
        motorObj();

        motorObj(int numMotors);

        motorObj(int numMotors, int* dir);

        ~motorObj();


        void slowOpen();
        void attachAll();
        void detachAll();
        bool isOpen();
        bool isBlindOpen();
        // angle to which to open the motor to.
        void setOpeningAngle(int angle=-1);
        void setBlindName(String name);

        void openOrCloseBlind();
        int getFeedback(int);
        long int getPosition(int);
        void cleanUpAfterSlowOpen();

        // get position of the slider converted to motor position
        int getPositionOfMotor(int sliderPos=-1);
        // get (current) position of motor translate to slider
        int getPositionOfSlider(int angle=-1);

        int getLimitFlag();

        String getBlindName();

        int getServoCount();

        void openBlinds();
        void closeBlinds();
        long int ifRunningHalt();

        int setWindowMax(int pos);  // does nothing
        int setWindowLow(int pos);  // does nothing

        void FactoryReset();
        void setSide(int direction, int index=0);
        void moveBlinds(int angle);

        void saveMotorParameters();  // save parameter of the various motors to the EEPROM
        void loadMotorParameters();
        bool isEEPROMRangeEmpty(int startAddress, int endAddress);
};

#endif