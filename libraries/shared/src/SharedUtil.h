//
//  SharedUtil.h
//  hifi
//
//  Created by Stephen Birarda on 2/22/13.
//
//

#ifndef __hifi__SharedUtil__
#define __hifi__SharedUtil__

#include <stdint.h>
#include <math.h>

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif

static const float	ZERO				= 0.0;
static const float	ONE					= 1.0;
static const float	ONE_HALF			= 0.5;
static const double	ONE_THIRD			= 0.3333333;
static const double	PIE					= 3.14159265359;
static const double	PI_TIMES_TWO		= 3.14159265359 * 2.0;
static const double PI_OVER_180			= 3.14159265359 / 180.0;
static const double EPSILON				= 0.00001;	//smallish number - used as margin of error for some computations
static const double SQUARE_ROOT_OF_2	= sqrt(2);
static const double SQUARE_ROOT_OF_3	= sqrt(3);
static const float METER				= 1.0;
static const float DECIMETER			= 0.1;
static const float CENTIMETER			= 0.01;
static const float MILLIIMETER			= 0.001;

double usecTimestamp(timeval *time);
double usecTimestampNow();

float randFloat();
int randIntInRange (int min, int max);
float randFloatInRange (float min,float max);
unsigned char randomColorValue(int minimum);
bool randomBoolean();

void outputBits(unsigned char byte);
void printVoxelCode(unsigned char* voxelCode);
int numberOfOnes(unsigned char byte);
bool oneAtBit(unsigned char byte, int bitIndex);

void switchToResourcesIfRequired();

const char* getCmdOption(int argc, const char * argv[],const char* option);
bool cmdOptionExists(int argc, const char * argv[],const char* option);

struct VoxelDetail {
	float x;
	float y;
	float z;
	float s;
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

unsigned char* pointToVoxel(float x, float y, float z, float s, unsigned char r, unsigned char g, unsigned char b );
bool createVoxelEditMessage(unsigned char command, short int sequence, 
        int voxelCount, VoxelDetail* voxelDetails, unsigned char*& bufferOut, int& sizeOut);

#ifdef _WIN32
void usleep(int waitTime);
#endif


#endif /* defined(__hifi__SharedUtil__) */
