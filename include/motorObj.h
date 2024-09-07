#ifndef MOTOR_OBJ_H
#define MOTOR_OBJ_H
#include <pwmWrite.h>

#define SERVO_MAX_ANGLE 180
#define SERVO_MIN_ANGLE  0

class motorObj{

    private:

        const int numServos;
        // servo Pins they are all standard
        int *sPin ;
        // readPins
        int *rPin ; 
        int *direction;

        float feedback;
        Pwm *myservo;
        bool servoAttached=false;
        double reading[20];
        long int highestPosition = SERVO_MAX_ANGLE;
        long int lowestPosition = SERVO_MIN_ANGLE;
        int maxOpenAngle = SERVO_MAX_ANGLE;
        int minOpenAngle = SERVO_MIN_ANGLE;
        bool blindsOpen=false;
        long int updateTime;
        int  limitFlag;
        const int sliderMin = 0;
        const int sliderMax = 100;
        String blindName;



    public:
        String status; // indicator of "open" or "close"

        motorObj(int numMotors=1);
        ~motorObj();
        void setDirections(int * dirs);
        void slowOpen();
        void attachAll();
        void detachAll();
        bool isOpen();
        bool isBlindOpen();
        void setOpeningAngle();
        void openOrCloseBlind();
        int getFeedback(int);
        long int getPosition(int);
        void cleanUpAfterSlowOpen();
        int getPositionOfMotor(int Pos=-1);
        int getPositionOfSlider(long int sliderPos=-1);
        int getLimitFlag();
        String getBlindName();
        void openBlinds();
        void closeBlinds();
        long int ifRunningHalt();

        int setWindowMax(int pos);  // does nothing
        int setWindowLow(int pos);  // does nothing

        void FactoryReset();
        void setSide(int direction, int index=0);
        void setBlindName(String blindname);
        void moveBlinds(int position);
};

#endif