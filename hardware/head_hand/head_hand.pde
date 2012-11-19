/*
Read a set of analog input lines and echo their readings over the serial port with averaging 
*/

//  ADC PIN MAPPINGS 
//
//  15,16 =   Head Pitch, Yaw gyro 
//  17,18,19    =  Head Accelerometer


#define NUM_CHANNELS 5
#define MSECS_PER_SAMPLE 10

int inputPins[NUM_CHANNELS] = {19,20,15,16,17};

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
      time = millis();
      for (i = 0; i < NUM_CHANNELS; i++) {
        measured[i] = accumulate[i] / sampleCount;
        SerialUSB.print(measured[i]);
        SerialUSB.print(" ");
        accumulate[i] = 0;
      }  
      //SerialUSB.print("(");
      //SerialUSB.print(sampleCount);
      //SerialUSB.print(")");
      SerialUSB.println("");
      sampleCount = 0;
  }  
}


  
