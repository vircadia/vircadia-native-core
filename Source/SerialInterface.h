//
//  SerialInterface.h
//  


#ifndef __interface__SerialInterface__
#define __interface__SerialInterface__

int init_port (int baud);
int read_sensors(int first_measurement, float * avg_adc_channels, int * adc_channels, int * samples_averaged, int * LED_state);

#define NUM_CHANNELS 6
#define SERIAL_PORT_NAME "/dev/tty.usbmodemfd131"

//  Acceleration sensors, in screen/world coord system (X = left/right, Y = Up/Down, Z = fwd/back)
#define ACCEL_X 4 
#define ACCEL_Y 5 
#define ACCEL_Z 3 

//  Gyro sensors, in coodinate system of head/airplane
#define PITCH_RATE 0 
#define YAW_RATE 1 
#define ROLL_RATE 2

#endif
