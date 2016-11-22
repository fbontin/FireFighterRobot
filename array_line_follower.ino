#include <QTRSensors.h>
#include <Servo.h>

#define NUM_SENSORS 6

Servo left;
Servo right;
int speed;
unsigned int sensors[NUM_SENSORS];


// sensors connected [X-XXXX-X]
QTRSensorsRC qtr((unsigned char[]) {2, 3, 4, 5, 6, 7}, NUM_SENSORS);

void setup() {
  Serial.begin(9600);
  left.attach(8);
  right.attach(9);
  speed = 10;

  setWheelSpeed(0, 0);

  Serial.println("Calibrating");
  for (int i = 0; i < 100; i++) {
    qtr.calibrate();
    delay(20);
  }
}

void loop() {
  unsigned int position = qtr.readLine(sensors);
  int error = position - 2500; //2500 is middle

  Serial.print("\nError: ");
  Serial.print(error);

  //if robot left of line  -->  error < 0
  //if robot right of line -->  error > 0 

  //float p = 0.01;
  float p = (float)(speed)/2500;  //2500 = max(error)
  float regSpeed = error * p; // -10 < regSpeed < 10

  int leftSpeed = speed - regSpeed;
  int rightSpeed = speed + regSpeed;

  Serial.print("\nRegspeed");
  Serial.print(regSpeed);


  Serial.print("\nLeft, Right: ");
  Serial.print(leftSpeed);
  Serial.print(" ");
  Serial.print(rightSpeed);

  Serial.print("\n");

  setWheelSpeed(leftSpeed, rightSpeed);

  /*
  //non p-regulator solution
  if (error < -500) { //turn right
    setWheelSpeed(speed, 1);
  } else if (error > 500) { //turn left
    setWheelSpeed(1, speed);
  } else { //forward
    setWheelSpeed(speed, speed);
  }
  */
  delay(10);
}

//sets speed for wheels
void setWheelSpeed(int leftSpeed, int rightSpeed) {
  left.write(90 + leftSpeed);
  right.write(90 - rightSpeed);
}

//checks if all sensors detect white
bool allWhite() {
  int threshold = 750;
  bool allWhite = true;
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (sensors[i] > threshold) {
      allWhite = false;
    }
  }
  return allWhite;
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

