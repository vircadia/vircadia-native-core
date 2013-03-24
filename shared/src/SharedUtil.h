//
//  SharedUtil.h
//  hifi
//
//  Created by Stephen Birarda on 2/22/13.
//
//

#ifndef __hifi__SharedUtil__
#define __hifi__SharedUtil__

#include <iostream>
#include <cstdio>

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif

double usecTimestamp(timeval *time);
double usecTimestampNow();

float randFloat();
float randFloatInRange (float min,float max);
unsigned char randomColorValue(int minimum);
bool randomBoolean();

void outputBits(unsigned char byte);
void printVoxelCode(unsigned char* voxelCode);
int numberOfOnes(unsigned char byte);
bool oneAtBit(unsigned char byte, int bitIndex);

void switchToResourcesIfRequired();

char* getCmdOption(char ** begin, char ** end, const std::string& option);
bool cmdOptionExists(char** begin, char** end, const std::string& option);

unsigned char* pointToVoxel(float x, float y, float z, float s, unsigned char r, unsigned char g, unsigned char b );

#endif /* defined(__hifi__SharedUtil__) */
