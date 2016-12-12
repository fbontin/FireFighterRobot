#include <QTRSensors.h>
#include <Servo.h>

#define NUM_SENSORS 6
#define THRESHOLD 170
#define SLOW_SPEED 5

Servo left;
Servo right;
int speed;
unsigned int sensors[NUM_SENSORS];
bool leftOnBlack, rightOnBlack, middleOnBlack;
String path;
bool replaceNext;
int eventDetectionCounter;
int candles;
int distances[2] = {20,20};

//Gripper
Servo gripperServo, lifterServo;
int speedGripper = 30;
int speedLifter = 20;

//Switch
const byte switchPinDown = 14;
const byte switchPinUp = 15;
volatile int lifterPosition = 0;

//Ultra Sonic Sensor
const int trigPin = 3;
const int echoPin = 2;
// Help variables
//long duration;
//int distance = 0;

//LEDS
const int ledPin =  16;
int ledState = LOW;
unsigned long previousMillis = 0;
const long interval = 500;


// sensors connected [X-XXXX-X]
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
  qtr.readLine(sensors);
  delay(20);
  
  leftOnBlack = rightOnBlack = middleOnBlack = false;
  path = "";
  replaceNext = false;
  candles = 0;

  eventDetectionCounter = 0;

  left.attach(4);
  right.attach(5);
  speed = 60;
  setWheelSpeed(0, 0);

  gripperServo.attach(13);
  gripperServo.write(91);
  lifterServo.attach(12);
  lifterServo.write(90);

  //Switch setup
  pinMode(switchPinDown, INPUT_PULLUP);
  pinMode(switchPinUp, INPUT_PULLUP);

  //Ultra Sonic setup
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  //Leds setup
  pinMode(ledPin, OUTPUT);
}

/*
 *  -------------------------- LOOP BEGIN ----------------------------
 */
void loop() {
  
  //event detection
 
  if ( eventDetectionCounter == 0 ) {
    int detected = detectEvent();
    printDetected(detected);
    act(detected);
    replacePath(detected);
  } else if (eventDetectionCounter >= 20) {
    eventDetectionCounter = 0;
  } else {
    eventDetectionCounter++;
  }
  
  regulate();
  
  if (candles < 3) {
    checkForCandle();
  }
  
  toggleLEDS();

  //setWheelSpeed(0, 0);
  //delay(20);
}

//when a crossing has been detected, this method acts (turns left, continues straight or whatever
void act(int detected) {

  if (detected != 0){
    Serial.println("Path: " + path);
  }

  if (candles < 3){
    
    switch (detected) {
      case 1: //T-cross, left or right
        turnLeft(true);
        break;
      case 2: //Right T
        goStraight(true);
        break;
      case 3: //Left T
        turnLeft(true);
        break;
      case 4: //4-way
        turnLeft(true);
        break;
      case 5: //Dead end
        replaceNext = true;
        uTurn(true);
        break;
      case 6: //A turn
        eventDetectionCounter = 1;
        break;
      default:
        break;
    }
  } else if (detected < 5 && detected > 0){

    if (path.length() != 0){
      char last = path.charAt(path.length()-1);
      path = path.substring(0, path.length()-1);


      Serial.println("Path: " + path + "[" + last + "]");

      switch (last) {
        case 'R':
          Serial.println("Going back, turning: L");
          turnLeft(false);
          break;
        case 'S':
          Serial.println("Going back, turning: S");
          goStraight(false);
          break;
        case 'L':
          Serial.println("Going back, turning: R");
          turnRight(false);
          break;
        default:
          break;
      }
      Serial.println("last: " + last);  
    }
  } else if (detected == 5 && path.length() < 2) {
    //AT THE END
    Serial.println("MAZE FINISHED");
    while(true) {
      setWheelSpeed(20, -20);
    }
  }
  
}

void replacePath(int detected) {
  if (replaceNext && detected < 5 && detected > 0) {
    String lastThree = path.substring(path.length() - 3);
    Serial.println("Path: " + path);
    Serial.println("Replacing: " + lastThree);
    if (lastThree == "LUS") {
      path = path.substring(0, path.length() - 3) + "R";
    } else if (lastThree == "SUL") {
      path = path.substring(0, path.length() - 3) + "R";
    } else if (lastThree == "LUL") {
      path = path.substring(0, path.length() - 3) + "S";
    } else {
      Serial.print("Tried to replace something that should not exist");
    }
    Serial.println("New path: " + path);
    replaceNext = false;
  }
}

void turnLeft(bool append) {
  Serial.println("Turning left");
  if(append) path += "L";
  setWheelSpeed(-10, 20);
  delayUntilOverLine();
}

void turnRight(bool append) {
  Serial.println("Turning right");
  if(append) path += "R";
  setWheelSpeed(20, -10);
  delayUntilOverLine();
}

void goStraight(bool append) {
  Serial.println("Going straight");
  if(append) path += "S";
  setWheelSpeed(20, 20);
  delay(200);
  return;
}

void uTurn(bool append) {
  Serial.println("U-turn");
  if(path.length() != 0){
    if (path.charAt(path.length() - 1) != 'U'){
        if(append) path += "U";
    }
  }
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
 *  Nothing  |  T-cross  |  RIght T  |  Left T  |  4-way  |  Dead End  | Turn (L or R)
 *     0           1           2          3          4           5             6
 */
int detectEvent(){

  //BLACK IS TRUE. TRUE IS BLACK
  qtr.readLine(sensors);
  leftOnBlack = sensors[5] > THRESHOLD;
  rightOnBlack = sensors[0] > THRESHOLD;
  middleOnBlack = sensors[2] > THRESHOLD || sensors[3] > THRESHOLD;
  bool outerMiddleOnBlack = sensors[1] > THRESHOLD || sensors[4] > THRESHOLD;

  if(!leftOnBlack && !rightOnBlack && !middleOnBlack && !outerMiddleOnBlack){
    return 5; //dead end
  }
  if ( !((leftOnBlack || rightOnBlack) && middleOnBlack) ) { 
    return 0; //nothing
  }
  
  bool prevLeft = leftOnBlack;
  bool prevRight = rightOnBlack;

  //move forward until what was black before becomes white
  setWheelSpeed(SLOW_SPEED + 3, SLOW_SPEED);
  while(leftOnBlack || rightOnBlack) { 
    delay(30);

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
  delay(700);

  qtr.readLine(sensors);
  //save middleValue. True if there is a road forward
  middleOnBlack = sensors[2] > THRESHOLD || sensors[3] > THRESHOLD;

  //--------------------Hmm, WTF?-----------------------------??
  //then, move back until they are black
  setWheelSpeed(-SLOW_SPEED, -SLOW_SPEED);
  while (leftOnBlack || rightOnBlack) {
    delay(30);
    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
  }
  
  //check what kind of crossing it is
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

  //if only turn, not crossing
  if (prevRight) turnLeft(false);
  else if (prevLeft) turnRight(false);
/*
  int leftSpeed = 0; 
  int rightSpeed = 0; 
  
  if (prevLeft) leftSpeed = 10;
  if (prevRight) rightSpeed = 10;

  setWheelSpeed(leftSpeed, rightSpeed);
*/
  /*
  //continue back until white again
  while (!leftOnBlack && !rightOnBlack) {
    delay(30); 
    qtr.readLine(sensors);
    leftOnBlack = sensors[5] > THRESHOLD;
    rightOnBlack = sensors[0] > THRESHOLD;
  }
  */
  return 6;
}

//sets speed for wheels
void setWheelSpeed(int leftSpeed, int rightSpeed) {
  left.write(94 - leftSpeed);
  right.write(90 + rightSpeed);
}

//Regulates to follow line and returns regulation error.
int regulate() {
  unsigned int position = qtr.readLine(sensors);
  int error = position - 2500; //2500 is middle
  float p = 0.048;  //2500 = max(error)
  float regSpeed = error * p;

  int leftSpeed = speed + regSpeed;
  int rightSpeed = speed - regSpeed;
 
  setWheelSpeed(leftSpeed, rightSpeed);
  return error;
}

//Prints the detected crossing.
void printDetected(int detected) {
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
          Serial.println("Dead end");
          break;
        default:
          break;
      }
  }
}

//prints values for all line sensors
void printAllLineSensors() {
  Serial.print("Line sensors: ");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(sensors[i]);
    Serial.print(" ");
  }
  Serial.print("\n");
}

/* ------------------- GRIPPER FUNCTIONS ------------------------------
 * --------------------------------------------------------------------
 * --------------------------------------------------------------------
 */

void checkForCandle(){
  
    //distance detection
    int distanceToObject = checkDistanceToObject();


    if(distances[1] < 6 && distances[1] > 1 && distances[0] < 6 && distances[0] > 1 && distanceToObject < 5 && distanceToObject > 1) {

      Serial.print("Close object detected at: ");
      Serial.println(checkDistanceToObject());
  
      Serial.println("Do lifting!");
      
      // TODO (2,2)?
      setWheelSpeed(3, 2);
      
      liftingSequence();
      
    }
    distances[1] = distances[0];
    distances[0] = distanceToObject;

    /*
    if (distanceToObject < 5) {
      
      Serial.print("Close object detected at: ");
      Serial.println(checkDistanceToObject());
  
      Serial.println("Do lifting!");
      setWheelSpeed(0, 0);
      liftingSequence();
    }
    */
}

int checkDistanceToObject() {

  long duration;
  int distance = 0;
  
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;

  return distance;
}

void liftingSequence() {
  delay(200);
  // Close to grip candle
  Serial.println("Close to grip candle");
  gripperServo.write(90 - speedGripper);
  delay(900);

  Serial.println("lift candle with lifterServo until switchUp button pressed");
  
  while (lifterPosition != 1) {
    readSwitchUp();
    lifterServo.write(90 - speedLifter);
  }

  delay(200);

  // Open gripper a certain delay
  Serial.println("Open gripper a certain delay");

  gripperServo.write(90 + speedGripper);
  delay(800);
  gripperServo.write(90);

  // Lower gripper with lifterServo until switchDown button pressed
  Serial.println("Lower gripper with lifterServo until switchDown button pressed");

  while (lifterPosition != 0) {
    readSwitchDown();
    lifterServo.write(90 + speedLifter);
  }
  lifterServo.write(90);
  //delay(1000);

  // Open gripper a certain delay

  Serial.println("Open gripper");
  gripperServo.write(90 + speedGripper);
  delay(400);
  gripperServo.write(91); //open slowly all the time!

  candles++;
  
  Serial.println("Done lifting!");
}

void toggleLEDS() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }
}

void readSwitchDown() {
  
  int value = digitalRead(14);

  if(value == HIGH){
    Serial.println("Down button pressed!");
    lifterPosition = 0;
  }
}

void readSwitchUp() {
  
  int value = digitalRead(15);

  if(value == HIGH){
    Serial.println("Up button pressed!");
    lifterPosition = 1;
  }
}


