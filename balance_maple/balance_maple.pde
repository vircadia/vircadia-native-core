/*
 Balance Platform code for Maple ret6

 Read a number of piezo pressure sensors at analog inputs and send data to PC 
 
*/

const int inputPinX = 0;
const int inputPinY = 1;
const int inputPinZ = 2;
const int inputPinW = 3;

int pingRcvd = 0; 

int corners[4];
int baseline[4];

float accum[4];

int sampleCount = 0;
const int NUM_SAMPLES=100;

const int debug = 1; 

unsigned int time;

char readBuffer[100]; 

void setup()
{
   pinMode(inputPinX, INPUT_ANALOG);
   pinMode(inputPinY, INPUT_ANALOG);
   pinMode(inputPinZ, INPUT_ANALOG);
   pinMode(inputPinW, INPUT_ANALOG);
   pinMode(BOARD_LED_PIN, OUTPUT);
   
   corners[0] = corners[1] = corners[2] = corners[3] = 0;
   accum[0] = accum[1] = accum[2] = accum[3] = 0.f;
   
   baseline[0] = analogRead(inputPinX);
   baseline[1] = analogRead(inputPinY);
   baseline[2] = analogRead(inputPinZ);
}

void loop()
{
   sampleCount++; 
   //  Read the instantaneous value of the pressure sensors, average over last 10 samples
   accum[0] += analogRead(inputPinX);
   accum[1] += analogRead(inputPinY);
   accum[2] += analogRead(inputPinZ);
   accum[3] += analogRead(inputPinW);
   //accum[2] += analogRead(inputPinZ);
  //corners[0] = accum[0];
  //corners[1] = accum[1];
  //corners[2] = accum[2];
  
   //  Periodically send averaged value to the PC
   //  Print out the instantaneous deviation from the trailing average
   if (sampleCount % NUM_SAMPLES == 0)
   {
     corners[0] = accum[0] / NUM_SAMPLES;
     corners[1] = accum[1] / NUM_SAMPLES;
     corners[2] = accum[2] / NUM_SAMPLES;
     corners[3] = accum[3] / NUM_SAMPLES;
     //corners[3] = accum[3] / NUM_SAMPLES;
     accum[0] = accum[1] = accum[2] = accum[3] = 0.f;
     
      if (debug) 
      {
        //SerialUSB.print("Measured = ");
        SerialUSB.print(corners[0]);
        SerialUSB.print(" ");
        SerialUSB.print(corners[1]);
        SerialUSB.print(" ");
        SerialUSB.print(corners[2]);
        SerialUSB.print(" ");
        SerialUSB.print(corners[3]);
        SerialUSB.println("");
      }
   }
   pingRcvd = 0;
   while (SerialUSB.available() > 0)
   {
       pingRcvd = 1;
       readBuffer[0] = SerialUSB.read();
   }
   if (pingRcvd == 1) 
   {
       SerialUSB.println("pong");
       toggleLED();
   }
}


  
