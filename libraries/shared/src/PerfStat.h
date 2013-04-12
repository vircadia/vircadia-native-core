//
//  HiFiPerfStat.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/29/13.
//
//  Poor-man's performance stats collector class. Useful for collecting timing
//  details from various portions of the code.
//
//

#ifndef __hifi__PerfStat__
#define __hifi__PerfStat__

#include <stdint.h>

#ifdef _WIN32
#define snprintf _snprintf
#include "Systime.h"
#else
#include <sys/time.h>
#endif

#include <cstring>
#include <string>
#include <map>

class PerfStatHistory {

private:
	long int count;
	double totalTime;
public:
	std::string group;
	
	PerfStatHistory(): count(0), totalTime(0.0) {};
	PerfStatHistory(std::string myGroup, double initialTime, long int initialCount) :
        count(initialCount), totalTime(initialTime), group(myGroup) {};
    
	void recordTime(double thisTime) { 
		totalTime+=thisTime; 
		count++; 
	};
	double getAverage() {
		return totalTime/count; 
	};
	double getTotalTime() {
		return totalTime; 
	};
	long int getCount() {
		return count; 
	};

	// needed for map template? Maybe not.
	bool operator<( const PerfStatHistory& other) const {
		return group < other.group;
	}
};

#define MAX_PERFSTAT_DEBUG_LINE_LEN 200

class PerfStat {
private:
	static std::map<std::string,PerfStatHistory,std::less<std::string> > groupHistoryMap;
	
	static timeval firstDumpTime;
	static bool firstDumpTimeSet;
	
	std::string group;
	timeval start;

public:
    PerfStat(std::string groupName);
    ~PerfStat();
    
    // Format debug stats into buffer, returns number of "lines" of stats
    static int DumpStats(char** array);
    static int getGroupCount();
    static bool wantDebugOut;
};

typedef std::map<std::string,PerfStatHistory,std::less<std::string> >::iterator PerfStatMapItr;


#endif /* defined(__hifi__PerfStat__) */
