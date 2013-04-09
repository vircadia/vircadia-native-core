//
//  CounterStats.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/29/13.
//
//  Poor-man's counter stats collector class. Useful for collecting running averages
//  and other stats for countable things. 
//
//

#ifndef __hifi__CounterStats__
#define __hifi__CounterStats__

#include <cstring>
#include <string>
#include <map>

// TIME_FRAME should be SAMPLES_TO_KEEP * TIME_BETWEEN_SAMPLES
#define COUNTETSTATS_SAMPLES_TO_KEEP 50
#define COUNTETSTATS_TIME_BETWEEN_SAMPLES 0.1
#define COUNTETSTATS_TIME_FRAME (COUNTETSTATS_SAMPLES_TO_KEEP*COUNTETSTATS_TIME_BETWEEN_SAMPLES)

class CounterStatHistory {

private:
	long int	currentCount;
	long int	currentDelta;
	double 		currentTime;

	long int	lastCount;
	double 		lastTime;
	
	double		totalTime;
	
	long int	countSamples[COUNTETSTATS_SAMPLES_TO_KEEP];
	long int	deltaSamples[COUNTETSTATS_SAMPLES_TO_KEEP];
	double		timeSamples[COUNTETSTATS_SAMPLES_TO_KEEP];
	int			sampleAt;
	int			sampleCount;
	
public:
	std::string name;

	CounterStatHistory(std::string myName): 
		currentCount(0), currentDelta(0),currentTime(0.0), 
		lastCount(0),lastTime(0.0), 
		totalTime(0.0), 
		sampleAt(-1),sampleCount(0), name(myName) {};
	
	CounterStatHistory(): 
		currentCount(0), currentDelta(0),currentTime(0.0), 
		lastCount(0),lastTime(0.0), 
		totalTime(0.0), 
		sampleAt(-1),sampleCount(0) {};
		
	CounterStatHistory(std::string myName, double initialTime, long int initialCount) :
        currentCount(initialCount), currentDelta(0), currentTime(initialTime), 
		lastCount(initialCount),lastTime(initialTime), 
        totalTime(initialTime), 
        sampleAt(-1), sampleCount(0), name(myName) {};
    
	void recordSample(long int thisCount);
	void recordSample(double thisTime, long int thisCount);
	long int getRunningAverage();

	long int getAverage() {
		return currentCount/totalTime; 
	};

	double getTotalTime() {
		return totalTime; 
	};
	long int getCount() {
		return currentCount; 
	};
};

#endif /* defined(__hifi__CounterStat__) */
