
#include <pwmWrite.h>

#define SERVO_MAX_ANGLE 180
#define SERVO_MIN_ANGLE  0

class motorObj{

    private:

        const int numServos=3;
        // servo Pins they are all standard
        const int sPin[3] = { 0, 1, 2 };
        // readPins
        const int rPin[3] = { 10,9,8 }; 
        int direction[3] = {1,1,1} ;

        float feedback;
        Pwm *myservo;
        bool servoAttached=false;
        double reading[20];
        int maxOpenAngle = SERVO_MAX_ANGLE;
        int minOpenAngle = SERVO_MIN_ANGLE;
        bool blindsOpen=false;

    public:
        motorObj();
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
        long getPosition(int);
        void cleanUpAfterSlowOpen();

};