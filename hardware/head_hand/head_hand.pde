/*
Read a set of analog input lines and echo their readings over the serial port with averaging 
*/

#define NUM_CHANNELS 8
#define AVERAGE_COUNT 100

int inputPins[NUM_CHANNELS] = {0,1,2,3,4,10,11,12};

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
}

void loop()
{
  int i;
  sampleCount++; 
  for (i = 0; i < NUM_CHANNELS; i++) {
    if (sampleCount % NUM_SAMPLES == 0) {
      measured[i] = accumulate[i] / AVERAGE_COUNT;
      SerialUSB.print(measured[i]);
      SerialUSB.print(" ");
      accumulate[i] = 0;
    } else {
      accumulate[i] += analogRead(inputPins[i]);
    }
  }
  if (sampleCount % NUM_SAMPLES == 0) SerialUSB.println("");
}


  
