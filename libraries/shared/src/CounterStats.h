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
public:
	std::string name;
    
	CounterStatHistory();
    CounterStatHistory(std::string myName);
	CounterStatHistory(std::string myName, double initialTime, long initialCount);
    
	void recordSample(long thisCount);
	void recordSample(double thisTime, long thisCount);
	long getRunningAverage();
    
	long getAverage() {
		return currentCount/totalTime;
	};
    
	double getTotalTime() {
		return totalTime;
	};
	long getCount() {
		return currentCount;
	};
private:
    void        init();
    
	long        currentCount;
	long        currentDelta;
	double 		currentTime;

	long        lastCount;
	double 		lastTime;
	
	double		totalTime;
	
	long        countSamples[COUNTETSTATS_SAMPLES_TO_KEEP];
	long        deltaSamples[COUNTETSTATS_SAMPLES_TO_KEEP];
	double		timeSamples[COUNTETSTATS_SAMPLES_TO_KEEP];
	int			sampleAt;
	int			sampleCount;
};

#endif /* defined(__hifi__CounterStat__) */
