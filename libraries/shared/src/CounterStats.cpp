//
//  CounterStats.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 2013/04/08.
//
//  Poor-man's counter stats collector class. Useful for collecting running averages
//  and other stats for countable things. 
//
//

#include "CounterStats.h"
#include <cstdio>

#ifdef _WIN32
#include "Systime.h"
#else
#include <sys/time.h>
#endif
#include <string>
#include <map>


//private:
//	long int	currentCount;
//  long int	currentDelta;
//	double 		currentTime;
//	double		totalTime;
//	
//	long int	countSamples[COUNTETSTATS_SAMPLES_TO_KEEP] = {};
//	long int	deltaSamples[COUNTETSTATS_SAMPLES_TO_KEEP] = {};
//	double		timeSamples[COUNTETSTATS_SAMPLES_TO_KEEP] = {};
//  int			sampleAt;


void CounterStatHistory::recordSample(long int thisCount) {
	timeval now;
	gettimeofday(&now,NULL);
	double nowSeconds = (now.tv_usec/1000000.0)+(now.tv_sec);
	this->recordSample(nowSeconds,thisCount);
}

void CounterStatHistory::recordSample(double thisTime, long int thisCount) {

	// how much did we change since last sample?
	long int thisDelta = thisCount - this->lastCount;
	double elapsed = thisTime - this->lastTime;

	// record the latest values
	this->currentCount = thisCount;
	this->currentTime = thisTime;
	this->currentDelta = thisDelta;

	//printf("CounterStatHistory[%s]::recordSample(thisTime %lf, thisCount= %ld)\n",this->name.c_str(),thisTime,thisCount);
	
	// if more than 1/10th of a second has passed, then record 
	// things in our rolling history
	if (elapsed > 0.1) {
		this->lastTime = thisTime;
		this->lastCount = thisCount;
	
		// record it in our history...
		this->sampleAt = (this->sampleAt+1)%COUNTETSTATS_SAMPLES_TO_KEEP;
		if (this->sampleCount<COUNTETSTATS_SAMPLES_TO_KEEP) {
			this->sampleCount++;
		}
		this->countSamples[this->sampleAt]=thisCount;
		this->timeSamples[this->sampleAt]=thisTime;
		this->deltaSamples[this->sampleAt]=thisDelta;

		//printf("CounterStatHistory[%s]::recordSample() ACTUALLY RECORDING IT sampleAt=%d thisTime %lf, thisCount= %ld)\n",this->name.c_str(),this->sampleAt,thisTime,thisCount);

	}
		
}

long int CounterStatHistory::getRunningAverage() {
	// before we calculate our running average, always "reset" the current count, with the current time
	// this will flush out old data, if we haven't been adding any new data.
	this->recordSample(this->currentCount);

	long int runningTotal = 0;
	double minTime = this->timeSamples[0];
	double maxTime = this->timeSamples[0];
	
	for (int i =0; i < this->sampleCount; i++) {
		minTime = std::min(minTime,this->timeSamples[i]);
		maxTime = std::max(maxTime,this->timeSamples[i]);
		runningTotal += this->deltaSamples[i];
	}
	
	double elapsedTime = maxTime-minTime;
	long int runningAverage = runningTotal/elapsedTime;
	return runningAverage;
}
