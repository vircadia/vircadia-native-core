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
unsigned char randomColorValue(uint8_t minimum);
bool randomBoolean();

void outputBits(unsigned char byte);
int8_t numberOfOnes(unsigned char byte);
bool oneAtBit(unsigned char byte, int8_t bitIndex);

#endif /* defined(__hifi__SharedUtil__) */
