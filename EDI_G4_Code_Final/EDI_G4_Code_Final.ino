/*LIBARIES*/
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <AccelStepper.h>
#include <HX711.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>


/*PORT DECLARATION*/
#define PIN        5          //For LED Strip
#define NUMPIXELS 12
#define dirPin 6              //For Stepper Motor, change to 6
#define stepPin 7             // Change to 7
#define motorInterfaceType 1
#define stepper_enable 8      // Change to 8
#define LOAD_DOUT 3           //For Load Cell
#define LOAD_SCK 4
#define brake_button A0
#define left_button 12
#define right_button 11
#define d_servo_port 9
#define power_out A1 //For checking power outage


/*OBJECT INSTANCES*/
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
HX711 cell;
Servo d_servo;
SoftwareSerial BT(0,1);

/*VARIABLES*/
int brake_state = 0;
int left_state = 1; // Pull-up resistor, therefore default (released) is 1
int right_state = 1;
int E_address = 0; // For EEPROM
int d_speed;

// Loadcell Neutal = 2230 - 2255, Variance ~25
int trailer_state = 2; // 1: Tension, 2: Neutral, 3: Compression

//For accelerometer
unsigned long prevMillis = 0;
int interval = 1000;




/*INITIALISATION/SET UP*/
void setup() {
  // put your setup code here, to run once:

  //User Input @ Control Panel
  pinMode(left_button, INPUT_PULLUP);
  pinMode(right_button, INPUT_PULLUP);
  pinMode(brake_button, INPUT);
  d_speed = EEPROM.read(E_address);

  //Serial.begin(9600);
  BT.begin(9600);

  // For stepper motor
  stepper.setMaxSpeed(1000);
  pinMode(stepper_enable, OUTPUT);
  //d_servo.attach(d_servo_port);

   // For Load Cell
  cell.begin(LOAD_DOUT, LOAD_SCK);
  cell.set_gain(64);

    
  pixels.begin(); // For Neopixel
  LED_signal("b"); // To warn the user not to move the device for callibration
  SM_turn("c", 1000, 500);
  delay(500);
  SM_turn("a", 900, 500);
  LED_signal("l");
  LED_signal("r");
  
  LED_signal("o"); // Indicate callibration is done
}



/*MAIN PROGRAMME*/
void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currentMillis = millis();
  float brake_func_state = brake_func();
  int power_check = analogRead(power_out)*5/1023; // Voltage measured

  if (power_check == 0){
    //Power is cut off, commence saving of last state
    EEPROM.update(E_address,d_speed); 
  }
  else{
    // Check if the left/right signal button is pressed
    
    turn_button(digitalRead(left_button), digitalRead(right_button));
  
    // Brake is pressed or released
    if (brake_func_state >= 650) {
      turn_button(0, 0); // Reset state of left&right signal button
      LED_signal("b");
      SM_turn("a", 1500, 500); // SM_turn (direction, position, speed);
      brake_state = 1;
    }
    else if (brake_func_state < 650 && brake_state == 1) {
      LED_signal("o");
      SM_turn("c", 700, 500);
     brake_state = 0;
    }
  
    // Check the load cell reading and adjust the speed every fixed time
    else if (currentMillis - prevMillis >= interval) {
      prevMillis = currentMillis;
      speed_adjustment();
      
    }
    BT_message(left_state,right_state,trailer_state); 
  }
  delay(50);

}








/*FUNCTIONS*/

/*=== Brake Force Sensitive Resistor ===*/
float brake_func() {
  float sum = 0;
  for (int i = 0; i < 24; i++) {
    float value = analogRead(brake_button);
    sum += value;
  }
  sum = sum / 25;
  //Serial.println(sum);
  return sum;
}


/*=== LED Signal ===*/
void LED_signal(char signal[]) {

  // Off State
  if (signal == "o" || signal == "O") {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0));
    }
    pixels.show();
  }

  // Brake Signal
  else if (signal == "b" || signal == "B") {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(150, 0, 0));
    }
    pixels.show();
  }

  // Turning Left
  else if (signal == "l" || signal == "L") {
    for (int i = 5; i > -1; i--) {
      pixels.setPixelColor(i, pixels.Color(150, 0, 0));
      pixels.show();
      delay(50);
    }
  }

  // Turning Right
  else if (signal == "r" || signal == "R") {
    for (int i = 6; i < 12; i++) {
      pixels.setPixelColor(i, pixels.Color(150, 0, 0));
      pixels.show();
      delay(50);
    }
  }
  pixels.clear();
}


/*=== Stepper Motor Turning ===*/
void SM_turn(char direction[], int position, int speed) {
  stepper.setCurrentPosition(0);
  digitalWrite(stepper_enable, LOW); // Turn on stepper coil

  // Clockwise
  if (direction == "c" || direction == "C") {
    while (stepper.currentPosition() != (-1 * position))
    {
      stepper.setSpeed((-1 * speed));
      stepper.runSpeed();
    }
  }

  // Anti-clockwise
  else if (direction == "a" || direction == "A") {
    while (stepper.currentPosition() != (position))
    {
      stepper.setSpeed((speed));
      stepper.runSpeed();
    }
  }
  digitalWrite(stepper_enable, HIGH); //Turn off stepper coil
}


/*=== Turn Button Trigger ===*/
void turn_button(int left, int right) {
  if ((right_state == 0 && left_state == 0) || (left == 0 && right == 0)) {
    left_state = 1;
    right_state = 1;
    LED_signal("o");
  }

  else {
    if (left == 0) {
      left_state = 0;
    }
    if (right == 0) {
      right_state = 0;
    }

    if (left_state == 0) {
      LED_signal("l");
    }
    else if (right_state == 0) {
      LED_signal("r");
    }
  }
  //Serial.print(left_state);
  //Serial.println(right_state);
}


/*===Derailleur Shifting===*/
/*Current Configuration:
  Speed 1: 175 (Higher Limit = 180)
  Speed 2: 165
  Speed 3: 155
  Speed 4: 145
  Speed 3: 135
  Speed 2: 125
  Speed 1: 115 (Lower Limit)*/
void d_shifting(char up_down[]) {
  
  if (up_down == "u" || up_down == "U") {
    if (d_speed < 3) {
      d_speed += 1;
    }
  }
  else if (up_down == "d" || up_down == "D") {
    if (d_speed > 1) {
      d_speed -= 1;
    }
  }
  //Serial.println(d_speed);

  switch (d_speed) {
    case 1:
      d_servo.write(175); // Gear 5
      //Serial.println(175);
      delay(500);
      break;
    case 2:
      d_servo.write(165); // Gear 4
      //Serial.println(145);
      delay(500);
      break;
    case 3:
      d_servo.write(155); // Gear 3
      //Serial.println(155);
      delay(500);
      break;
  }
}


/*===Cell Reading + Speed Adjustment===*/
void speed_adjustment() {
  float cell_UL = -138000; // Upper Limit for load cell to up gear (increase speed, reduce torque, harder to cycle)
  float cell_LL = -150000; // Lower Limit for load cell to down gear (reduce speed, increase torque, easier to cycle)
  float cell_val = cell.read_average(10);
  Serial.print(cell_val);
  if (cell_val >= cell_UL) {
    //Serial.println(" Too fast");
    trailer_state = 3;
    d_shifting("u");
    delay(2000);
    SM_turn("a", 1500, 500); // SM_turn (direction, position, speed);
    delay(200);
    SM_turn("c", 200, 500);
    
  }
  else if (cell_val <= cell_LL) {
    //Serial.println(" Too slow");
    trailer_state = 1;
    d_shifting("d");
  }
  else{
    trailer_state = 2;
    //Serial.println(" Acceptable");
  }

}


/*===BT Message ===*/ 
/*Need to modify, there is a lag*/
void BT_message(int left_turn, int right_turn, int trailer){
  char turning_message[6];
  char trailer_message[8];
  if (left_turn ==0){
    strcpy(turning_message,"Left");
  }
  else if(right_turn ==0){
    strcpy(turning_message,"Right");
  }
  else{
    strcpy(turning_message,"---");
  }

  if(trailer == 1){
    strcpy(trailer_message,"Slow");
  }
  else if(trailer == 2){
    strcpy(trailer_message,"In Sync");
  }
  else if(trailer == 3){
    strcpy(trailer_message,"Fast");
  }

  
  BT.print(turning_message);
  BT.print(","); 
  BT.print(d_speed);
  BT.print(";");
}
