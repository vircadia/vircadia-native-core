//
//  SerialInterface.h
//  


#ifndef interface_SerialInterface_h
#define interface_SerialInterface_h

int init_port (int baud);
int read_sensors(int first_measurement, float * avg_adc_channels, int * adc_channels);

const int CHANNEL_COUNT = 4;

class SerialInterface {
    int _serial_fd; // internal device id
    bool _enabled;  // Enables/disables serial i/o
    				// Disabled by default if open() fails

    // Internal persistant state for readSensors()
    char _buffer [CHANNEL_COUNT * sizeof(int)];
    int _bufferPos; // = 0;
    bool _readPacket;
    
    // Try to open the serial port. On failure, disable the interface internally, write
    // error message to cerr, and return non-zero error code (which can be ignored).
    // Called by constructor and enable().
    int initInterface();
    
    // Close the serial port.
    // Called by deconstructor and disable().
    void closeInterface();

public:
	SerialInterface() 
		: _enabled(true), 
		  _bufferPos(0), 
		  _readPacket(false)
	{
		initInterface();
	}
	~SerialInterface() {
		closeInterface();
	}
    
    // Try to reinitialize the interface.
    // If already enabled, do nothing. 
    // If reinitialization fails return false; otherwise return true.
	bool enable();
    
    // Disable the interface.
	void disable();
    
    bool isEnabled () { return _enabled; }
    
    // Updates gyro using serial input.
    // Reads data from serial port into _buffer and accumulates that into _channels.
    // Uses delta time and _channel input to update gyro yaw, pitch, and roll
    void readSensors (float deltaTime);
};


#endif
