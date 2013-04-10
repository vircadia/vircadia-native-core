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

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif

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
