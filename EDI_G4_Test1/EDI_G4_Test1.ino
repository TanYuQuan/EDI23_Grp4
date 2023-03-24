/*Libaries*/
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <MPU6050_tockn.h>
#include <Adafruit_Sensor.h>
#include <AccelStepper.h>
#include <HX711.h>

/*Port Declaration*/
#define PIN        5          //For LED Strip
#define NUMPIXELS 10
#define dirPin 7              //For Stepper Motor
#define stepPin 8
#define motorInterfaceType 1
#define stepper_enable 6
#define LOAD_DOUT 3           //For Load Cell
#define LOAD_SCK 4

/*Object Instances*/
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
MPU6050 mpu6050(Wire, 0.2,0.8);
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);
HX711 cell;

/*Variables*/
int brake_state = 1;


void setup() {
  // put your setup code here, to run once:
  pinMode(13,OUTPUT);
  digitalWrite(13,HIGH); // Tell user it is calibrating
  delay(1000);
  
  Serial.begin(9600);

  // For MPU6050
  Wire.begin();
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true); 

  // For Load Cell
  cell.begin(LOAD_DOUT,LOAD_SCK);
  cell.tare();
  
  pixels.begin(); // For Neopixel

  // For stepper motor
  stepper.setMaxSpeed(1000);
  pinMode(stepper_enable,OUTPUT);

  digitalWrite(13, LOW); // Tell user calibration over
}

/*Main Program*/
void loop() {
  // put your main code here, to run repeatedly:
  float cell_reading = cell.read_average(4);
  Serial.println(cell_reading);

  mpu6050.update();
  float yaw = mpu6050.getAngleZ();
  //float roll = mpu6050.getAngleX();
  //float pitch = mpu6050.getAngleY(); 
  /*
  Serial.print("angleX : ");
  Serial.print(roll);
  Serial.print("\tangleY : ");
  Serial.print(pitch);
  Serial.print("\tangleZ : ");
  Serial.println(yaw);
  */

  stepper.setCurrentPosition(0);
  if (cell_reading< 268000){
    digitalWrite(stepper_enable,LOW);
    while(stepper.currentPosition() != -200)
    {
      stepper.setSpeed(-200);
      stepper.runSpeed();
    }
    digitalWrite(stepper_enable,HIGH); //Turn off stepper coil
    brake_state = 1;
  }

  else if(cell_reading >=268000 && brake_state == 1) {
    digitalWrite(stepper_enable,LOW);
    while(stepper.currentPosition() != 200)
    {
      stepper.setSpeed(200);
      stepper.runSpeed();
    }
    digitalWrite(stepper_enable,HIGH); //Turn off stepper coil
    brake_state = 0;
  }

  if (yaw >= 10){
    right_turn();
  }
  else if(yaw <= -10){
    left_turn();
  }
  else{
    LED_Off();
  }
  
  delay(10);
  
}





/*Functions*/
void left_turn(){
  for (int i=4;i>-1;i--){
  pixels.setPixelColor(i, pixels.Color(150, 0, 0));
  pixels.show();
  delay(100);
  }
  pixels.clear();
}

void right_turn(){
  for (int i=5;i<10;i++){
  pixels.setPixelColor(i, pixels.Color(150, 0, 0));
  pixels.show();
  delay(100);
  }
   pixels.clear();
}

void brake_light(){
  for (int i=0;i<NUMPIXELS;i++){
  pixels.setPixelColor(i, pixels.Color(150, 0, 0));
  }
  pixels.show();
  delay(500);
}

void LED_Off(){
  for (int i=0;i<NUMPIXELS;i++){
  pixels.setPixelColor(i, pixels.Color(0, 0, 0));
  }
  pixels.show();
}

