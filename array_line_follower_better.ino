#include <QTRSensors.h>
#include <Servo.h>

#define NUM_SENSORS 6
#define THRESHOLD 750
#define SLOW_SPEED 5

Servo left;
Servo right;
int speed;
unsigned int sensors[NUM_SENSORS];
bool leftOnBlack, rightOnBlack, middleOnBlack;
String path;
bool replaceNext;

// sensors connected [X-XXXX-X]
//QTRSensorsRC qtr((unsigned char[]) {2, 3, 4, 5, 6, 7}, NUM_SENSORS);


//maybe working????
QTRSensorsRC qtr((unsigned char[]) {6, 7, 8, 9, 10, 11}, NUM_SENSORS);


void setup() {
  Serial.begin(9600);
  
  Serial.println("Calibrating");
  
  //int calSpeed = 5;
  //setWheelSpeed(calSpeed, -calSpeed);
  for (int i = 0; i < 100; i++) {
    /*
    if (i == 25) {setWheelSpeed(-calSpeed, calSpeed);}
    if (i == 50) {setWheelSpeed(calSpeed, -calSpeed);}
    if (i == 75) {setWheelSpeed(-calSpeed, calSpeed);}
    */
    qtr.calibrate();
    delay(20);
  }  
  Serial.println("Calibration finished");
  
  leftOnBlack = rightOnBlack = middleOnBlack = false;
  path = "";
  replaceNext = false;

  left.attach(4);
  right.attach(5);
  speed = 20;
  setWheelSpeed(0, 0);
}

/*
 *  -------------------------- LOOP BEGIN ----------------------------
 */
void loop() {

  // ------------- LOWER THRESHOLD PROBABLy ----------------------------
  // or at least try a lower value.
  
  //PRINT DETECTION
  int detected = detectEvent();
  if (detected > 0) {
      switch (detected) {
        case 1: 
          Serial.println("T-cross");
          break;
        case 2: 
          Serial.println("Right T-cross");
          break;
        case 3: 
          Serial.println("Left T-cross");
          break;
        case 4: 
          Serial.println("4-way");
          break;
        case 5: 
          Serial.println("DEAD end");
          break;
        default:
          Serial.println("Something is fucked");
          break;
      }
  }
  //TODO: remember crossing (stack or something)
  act(detected);

  //Regulation and steering
  unsigned int position = qtr.readLine(sensors);
  int error = position - 2500; //2500 is middle
  float p = 0.016;  //2500 = max(error)
  float regSpeed = error * p;

  int leftSpeed = speed + regSpeed;
  int rightSpeed = speed - regSpeed;
 
  setWheelSpeed(leftSpeed, rightSpeed);
  
  delay(20);
}

//when a crossing has been detected, this method acts (turns left, continues straight or whatever
void act(int detected) {
  switch (detected) {
    case 1: //T-cross, left or right
      turnLeft();
      break;
    case 2: //Right T
      goStraight();
      break;
    case 3: //Left T
      turnLeft();
      break;
    case 4: //4-way
      turnLeft();
      break;
    case 5: //Dead end
      replaceNext = true;
      uTurn();
      break;
    default:
      break;
  }
  if (replaceNext && detected < 5 && detected > 0) {
    String lastThree = path.substring(path.length() - 3);
    Serial.println("REPLACING" + lastThree);
    if (lastThree == "LUS") {
      path = path.substring(0, path.length() - 3) + "R";
    } else if (lastThree == "SUL") {
      path = path.substring(0, path.length() - 3) + "R";
    } else if (lastThree == "LUL") {
      path = path.substring(0, path.length() - 3) + "S";
    } else {
      Serial.print("Sometin is focked wid replacin");
    }
    replaceNext = false;
  }
}

void turnLeft() {
  Serial.println("Turning left");
  path += "L";
  setWheelSpeed(-10, 20);
  delayUntilOverLine();
}

void turnRight() {
  Serial.println("Turning right");
  path += "R";
  setWheelSpeed(20, -10);
  delayUntilOverLine();
}

void goStraight() {
  Serial.println("Going straight");
  path += "S";
  //how do it here?
  setWheelSpeed(20, 20);
  delay(800);
  return;
}

void uTurn() {
  Serial.println("U-turn");
  path += "U";
  setWheelSpeed(-20, 20);
  delayUntilOverLine();
}

//delays until line sensor is over line
void delayUntilOverLine() {
  delay(700); //so that it won't read the original line.
  int error = 2001;
  while (error > 2000 || error < -2000) {
      error = qtr.readLine(sensors) - 2500;
      delay(20);
  }
}

/* --------------------- DETECT EVENT ----------------------------------
 * Detects events. Return correct info. Does not choose path or move on.
 *
 *  Nothing  |  T-cross  |  RIght T  |  Left T  |  4-way  |  Dead End
 *     0           1           2          3          4           5
 */
int detectEvent(){

  //BLACK IS TRUE. TRUE IS BLACK
  leftOnBlack = sensors[5] > THRESHOLD;
  rightOnBlack = sensors[0] > THRESHOLD;
  middleOnBlack = sensors[2] > THRESHOLD || sensors[3] > THRESHOLD;
  bool outerMiddleOnBlack = sensors[1] > THRESHOLD || sensors[4] > THRESHOLD;

  printAllLineSensors();

  if(!leftOnBlack && !rightOnBlack && !middleOnBlack && !outerMiddleOnBlack){
    return 5; //dead end
  }
  if ( !((leftOnBlack || rightOnBlack) && middleOnBlack) ) { 
    return 0; //nothing
  }
  
  bool prevLeft = leftOnBlack;
  bool prevRight = rightOnBlack;

  //move forward until what was black before becomes white
  setWheelSpeed(SLOW_SPEED, SLOW_SPEED);
  
  while(leftOnBlack || rightOnBlack) { 
    delay(30); //maybe??

    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
    if (leftOnBlack) {
      prevLeft = true;
    }
    if (rightOnBlack) {
      prevRight = true;
    }
  }
  delay(500);

  //save middleValue. True if there is a road forward
  middleOnBlack = sensors[2] > THRESHOLD || sensors[3] > THRESHOLD;

  //then, move back until they are black
  setWheelSpeed(-SLOW_SPEED, -SLOW_SPEED);
  while (leftOnBlack || rightOnBlack) {
    delay(30);
    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
  }

  //continue back until white again
  while (!leftOnBlack && !rightOnBlack) {
    delay(30); 
    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
  }
  
  setWheelSpeed(0, 0);

  if (prevLeft && prevRight && !middleOnBlack) {
    return 1; // T-cross
  } 
  else if (prevLeft && !prevRight && middleOnBlack) {
    return 2; // Right T
  }
  else if (!prevLeft && prevRight && middleOnBlack) {
    return 3; // Left T
  }
  else if (prevLeft && prevRight && middleOnBlack) {
    return 4; // 4-way
  }
  //leftOnBlack = rightOnBlack = middleOnBlack = false;
  return 0;
}

//sets speed for wheels
void setWheelSpeed(int leftSpeed, int rightSpeed) {
  left.write(90 - leftSpeed);
  right.write(90 + rightSpeed);
}

//prints values for all line sensors
void printAllLineSensors() {
  Serial.print("\n");
  Serial.print("Line sensors: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(sensors[i]);
    Serial.print(" ");
  }
}
