// Gripper
#include <Servo.h>
Servo gripperServo, lifterServo;
int speedGripper = 30;
int speedLifter = 20;


// Switch
const byte switchPinDown = 14;
const byte switchPinUp = 15;
volatile int lifterPosition = 0; //0 is lifter position down, 1 is lifter position up

//Ultra Sonic Sensor
const int trigPin = 3;
const int echoPin = 2;
// Help variables
long duration;
int distance = 0;


void setup() {

  delay(3000);
  //Gripper setup
  
    gripperServo.attach(13);
    gripperServo.write(90);
    lifterServo.attach(12);
    lifterServo.write(90);
  

  Serial.begin(9600);

  
    //Down switch setup
    pinMode(switchPinDown, INPUT_PULLUP);

    //Up switch setup
    pinMode(switchPinUp, INPUT_PULLUP);
    

  
  //Ultra Sonic setup
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  


}

int checkDistanceToObject() {
  
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

  // Make sure gripper is open

  Serial.println("Open gripper");
  gripperServo.write(90 + speedGripper);
  delay(800);

  // Close to grip candle

  Serial.println("Close to grip candle");
  gripperServo.write(90 - speedGripper);
  delay(900);
  // lift candle with lifterServo until switchUp button pressed

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
  delay(1000);

  // Open gripper a certain delay

  Serial.println("Open gripper");
  gripperServo.write(90 + speedGripper);
  delay(800);
  gripperServo.write(90);

  Serial.println("Done lifting!");
}

void loop() {

  
  int distanceToObject = checkDistanceToObject();
  //Serial.println(distanceToObject);
  
  if (distanceToObject < 5) {
    Serial.print("Close object detected at: ");
    Serial.println(checkDistanceToObject());

    Serial.println("Do lifting!");
    liftingSequence();
    
  }

  delay(3000);
}

void readSwitchDown() {
  
  int value = digitalRead(14);

  if(value == HIGH){
    Serial.println("Down button pressed!");
    lifterPosition = 0;
  } else if(value == LOW) {
    Serial.println("Down not Pressed");
  }
  
}

void readSwitchUp() {
  
  int value = digitalRead(15);

  if(value == HIGH){
    Serial.println("Up button pressed!");
    lifterPosition = 1;
  } else if(value == LOW) {
    Serial.println("Up not Pressed");
  }
  
}


