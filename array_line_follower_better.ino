#include <QTRSensors.h>
#include <Servo.h>

#define NUM_SENSORS 6
#define THRESHOLD 750
#define SLOW_SPEED 5

Servo left;
Servo right;
int speed;
unsigned int sensors[NUM_SENSORS];
//bool* values[NUM_SENSORS] = {false, false, false, false, false, false};

bool leftOnBlack, rightOnBlack, middleOnBlack;


// sensors connected [X-XXXX-X]
QTRSensorsRC qtr((unsigned char[]) {2, 3, 4, 5, 6, 7}, NUM_SENSORS);

void setup() {
  Serial.begin(9600);

  Serial.println("Calibrating");
  for (int i = 0; i < 100; i++) {
    qtr.calibrate();
    delay(20);
  }
  
  left.attach(8);
  right.attach(9);
  speed = 20;
  leftOnBlack = rightOnBlack = middleOnBlack = false;
  setWheelSpeed(0, 0);
}

/*
 *  -------------------------- LOOP BEGIN ----------------------------
 */


void loop() {
  unsigned int position = qtr.readLine(sensors);
  int error = position - 2500; //2500 is middle

  //if robot left of line  -->  error < 0
  //if robot right of line -->  error > 0 

  float p = 0.016;  //2500 = max(error)
  float regSpeed = error * p; // -10 < regSpeed < 10

  int leftSpeed = speed - regSpeed;
  int rightSpeed = speed + regSpeed;
 
  setWheelSpeed(leftSpeed, rightSpeed);

  //int kuken = 0;
  //setWheelSpeed(kuken, kuken);
  // ------------------------------------------------------------------------------------ NEW CODE; TEST OUTPUT PLS. -----------------------------------------------------------------------------------------------------------------
  // Left T and Right T should swift places
  // Does not notice left turns
  int detected = detectEvent();
  if (detected > 0) {
      Serial.print("\nDETECT EVENT: ");
      Serial.print(detected);
  }


  delay(20);
}

/* --------------------- DETECT EVENT ----------------------------------
 * Detects events. Return correct info. Does not choose path or move on.
 *
 *  Nothing  |  T-cross  |  Left T  |  Right T  |  4-way  |  Dead End
 *     0           1           2          3          4           5
 */
int detectEvent(){

  //BLACK IS TRUE. TRUE IS BLACK
  leftOnBlack = sensors[5] > THRESHOLD;
  rightOnBlack = sensors[0] > THRESHOLD;
  middleOnBlack = sensors[2] > THRESHOLD || sensors[3] > THRESHOLD;

  if ( !((leftOnBlack || rightOnBlack) && middleOnBlack) ) { 
    return 0; //nothing
  } else if(!leftOnBlack && !rightOnBlack && !middleOnBlack){
    return 5; //dead end
  }
  
  bool prevLeft = leftOnBlack;
  bool prevRight = rightOnBlack;

  //move forward until what was black before becomes white
  setWheelSpeed(SLOW_SPEED, SLOW_SPEED);
  while (prevLeft == leftOnBlack && prevRight == rightOnBlack) {

    delay(30); //maybe??

    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
  }

  delay(500);

  //save middleValue. True if road forward
  middleOnBlack = sensors[2] > THRESHOLD || sensors[3] > THRESHOLD;

  //then, move back until they are black
  setWheelSpeed(-SLOW_SPEED, -SLOW_SPEED);
  while (prevLeft != leftOnBlack || prevRight != rightOnBlack) {
    delay(30); //maybe??

    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
  }

  //continue back until white again
  while (prevLeft == leftOnBlack && prevRight == rightOnBlack) {
    delay(30); //maybe??

    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
  }
  setWheelSpeed(0, 0);

  if (prevLeft && prevRight && !middleOnBlack) {
    return 1; // T-cross
  } 
  else if (prevLeft && !prevRight && middleOnBlack) {
    return 2; // Left T
  }
  else if (!prevLeft && prevRight && middleOnBlack) {
    return 3; // Right T
  }
  else if (prevLeft && prevRight && middleOnBlack) {
    return 4; // 4-way
  }
  
  leftOnBlack = rightOnBlack = middleOnBlack = false;
  return 0;
}

//sets speed for wheels
void setWheelSpeed(int leftSpeed, int rightSpeed) {
  left.write(90 + leftSpeed);
  right.write(90 - rightSpeed);
}

//prints values for all line sensors
void printAllLineSensors() {
    for (int i = 0; i < NUM_SENSORS; i++) {
      if (i == 0) {
        Serial.print("\n");
        Serial.print("Line sensors: ");
      }
      Serial.print(sensors[i]);
      Serial.print(" ");
    }
}
