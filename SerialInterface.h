//
//  SerialInterface.h
//  


#ifndef interface_SerialInterface_h
#define interface_SerialInterface_h

int init_port (int baud);
int read_sensors(int first_measurement, float * avg_adc_channels, int * adc_channels);

#define NUM_CHANNELS 8
#define SERIAL_PORT_NAME "/dev/tty.usbmodem411"

#endif
