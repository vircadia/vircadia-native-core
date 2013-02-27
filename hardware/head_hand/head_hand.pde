//
//  Read Gyro and accelerometer data, send over serialUSB to computer for processing.
//
//  Written by Philip, 2012, for High Fidelity, Inc. 
//  
//  PIN WIRING:  Connect input sensors to the channels in following manner
//
//  SDA:     Magnetomer SDA
//  SDL:     Magnetomer SDL
//  AIN 10:  Yaw Gyro (shaking your head 'no')
//  AIN 16:  Pitch Gyro (nodding your head 'yes') 
//  AIN 17:  Roll Gyro (looking quizzical, tilting your head)
//  AIN 18:  Lateral acceleration (moving from side-to-side in front of your monitor)
//  AIN 19:  Up/Down acceleration (sitting up/ducking in front of your monitor)
//  AIN 20:  Forward/Back acceleration (Toward or away from your monitor)

// include HardWire for I2C communication to Magnetomer
#include <HardWire.h>

#define NUM_CHANNELS 6
#define MSECS_PER_SAMPLE 10

#define LED_PIN 12

const int COMPASS_ADDRESS = 0x42 >> 1;

int inputPins[NUM_CHANNELS] = {10,16,17,18,19,20};

int LED = 0;
unsigned int samplesSent = 0;
unsigned int time;

int measured[NUM_CHANNELS];
float accumulate[NUM_CHANNELS];

bool readFromMag = false;

int sampleCount = 0;

HardWire Wire(1, I2C_FAST_MODE);

void setup()
{
  int i;
  for (i = 0; i < NUM_CHANNELS; i++) {
    pinMode(inputPins[i], INPUT_ANALOG);
    measured[i] = analogRead(inputPins[i]);
    accumulate[i] = measured[i];
  }
  Wire.begin();
  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  time = millis();
}

void loop()
{
  int i;
  int compassReading;
  bool magTransmit = false;
  sampleCount++;
  
  for (i = 0; i < NUM_CHANNELS; i++) {
    accumulate[i] += analogRead(inputPins[i]);
  }
  
  if (readFromMag && magTransmit == false) {
     // get current absolute magnetometer value
     Wire.beginTransmission(COMPASS_ADDRESS);
     Wire.send('A');
     Wire.endTransmission();
     magTransmit = true;
  }
  
  if ((millis() - time) >= MSECS_PER_SAMPLE) {
      samplesSent++;
      time = millis();
      for (i = 0; i < NUM_CHANNELS; i++) {
        measured[i] = accumulate[i] / sampleCount;
        SerialUSB.print(measured[i]);
        SerialUSB.print(" ");
        accumulate[i] = 0;
      }
            
      if ((samplesSent % 100 == 0) && (samplesSent % 150 == 0))  {
        LED = !LED;
        digitalWrite(LED_PIN, LED);
        digitalWrite(BOARD_LED_PIN, LED);
      } 
  
      SerialUSB.print(sampleCount);
      SerialUSB.print(" ");
      if (LED) 
        SerialUSB.print("1");
      else 
        SerialUSB.print("0");
        
      SerialUSB.print(" ");
      sampleCount = 0;
      
      if (readFromMag) {
        // send the absolute value from the magnetomer
        Wire.requestFrom(COMPASS_ADDRESS, 2);
      
        if (2 <= Wire.available()) {
  		    compassReading = Wire.receive();
  		    compassReading = compassReading << 8;
  		    compassReading += Wire.receive();
  	      compassReading /= 10;
  	      SerialUSB.print(compassReading);   
        }
      }
      
      SerialUSB.println("");
      
      // reset the magnetomerTransmit to false to prep it in next loop
      magTransmit = false;
  }  
}