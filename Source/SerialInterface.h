//
//  SerialInterface.h
//  


#ifndef interface_SerialInterface_h
#define interface_SerialInterface_h

int init_port (int baud);
int read_sensors(int first_measurement, float * avg_adc_channels, int * adc_channels, int * samples_averaged, int * LED_state);

#define NUM_CHANNELS 6
#define SERIAL_PORT_NAME "/dev/tty.usbmodem1411"

//  Acceleration sensors, in screen/world coord system (X = left/right, Y = Up/Down, Z = fwd/back)
#define ACCEL_X 4 
#define ACCEL_Y 5 
#define ACCEL_Z 3 

//  Gyro sensors, in coodinate system of head/airplane
#define PITCH_RATE 0 
#define YAW_RATE 1 
#define ROLL_RATE 2

#endif
