//
//  Read Gyro and accelerometer data, send over serialUSB to computer for processing.
//
//  Written by Philip, 2012, for High Fidelity, Inc. 
//  
//  PIN WIRING:  Connect input sensors to the channels in following manner
//
//  AIN 10:  Yaw Gyro (shaking your head 'no')
//  AIN 16:  Pitch Gyro (nodding your head 'yes') 
//  AIN 17:  Roll Gyro (looking quizzical, tilting your head)
//  AIN 18:  Lateral acceleration (moving from side-to-side in front of your monitor)
//  AIN 19:  Up/Down acceleration (sitting up/ducking in front of your monitor)
//  AIN 20:  Forward/Back acceleration (Toward or away from your monitor)

#define NUM_CHANNELS 6
#define MSECS_PER_SAMPLE 10

#define LED_PIN 12

int inputPins[NUM_CHANNELS] = {10,16,17,18,19,20};

int LED = 0;
unsigned int samplesSent = 0;
unsigned int time;

int measured[NUM_CHANNELS];
float accumulate[NUM_CHANNELS];

int sampleCount = 0;

void setup()
{
  int i;
  for (i = 0; i < NUM_CHANNELS; i++) {
    pinMode(inputPins[i], INPUT_ANALOG);
    measured[i] = analogRead(inputPins[i]);
    accumulate[i] = measured[i];
  }
  pinMode(BOARD_LED_PIN, OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  time = millis();
}

void loop()
{
  int i;
  sampleCount++;
  
  for (i = 0; i < NUM_CHANNELS; i++) {
    accumulate[i] += analogRead(inputPins[i]);
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
        
      SerialUSB.println("");
      sampleCount = 0;
  }  
}


  
