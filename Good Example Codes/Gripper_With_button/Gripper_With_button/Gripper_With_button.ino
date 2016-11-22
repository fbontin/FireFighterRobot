// Gripper
#include <Servo.h>
Servo gripperServo, lifterServo;
int speedGripper = 20;
int speedLifter = 10;


// Switch
const byte interruptPin = 2;
volatile int switchPressed = 0;

int doOnce = 0;

void setup(){

  //gripperServo.attach(8);
  //gripperServo.write(90);
  lifterServo.attach(9);
  lifterServo.write(90);
  
  Serial.begin(9600);
  
  //pinMode(interruptPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), switchStateChanged, CHANGE);
  
}

void liftingSequence() {

  
  // Close to grip candle

  Serial.println("Close to grip candle");
  gripperServo.write(90 + speedGripper);
  delay(700);
  
  // lift candle with lifterServo a certain degree (encoder)

  Serial.println("lift candle with lifterServo a certain degree");
  
  lifterServo.write(90 - speedLifter);
  delay(200);
  lifterServo.write(90);
  delay(400);
  

  // Open gripper until button pressed (interrupt)
  Serial.println("Open gripper until button pressed (interrupt)");
  while(switchPressed != 1) {
    gripperServo.write(90 - speedGripper);
  }
  
  gripperServo.write(90);

  // Lower gripper with lifterServo a certain degree (encoder)
  Serial.println("Lower gripper with lifterServo a certain degree");
  lifterServo.write(90 + speedLifter);
  delay(200);
  lifterServo.write(90);
  Serial.println("Done lifting!");
}
 
void loop(){

  /*
  switchPressed = digitalRead(interruptPin);
  */

  /*
  if(doOnce != 1) {
    Serial.println("Do lifting!");
    liftingSequence();
    doOnce = 1;
  } else {
    Serial.println("Already done with the lift!");
    lifterServo.write(90);
    gripperServo.write(90);
  }
  */
  
  delay(2000);
}

void switchStateChanged() {
  Serial.println("button pressed!");
  
  if(switchPressed == 1) {
    switchPressed = 0;
    Serial.println("New value is 0");
  } else {
    switchPressed = 1;
    Serial.println("New value is 1");
  }

}
