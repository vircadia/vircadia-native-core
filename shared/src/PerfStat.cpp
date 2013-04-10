//
//  HiFiPerfStat.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 3/29/13.
//
//  Poor-man's performance stats collector class. Useful for collecting timing
//  details from various portions of the code.
//
//

#include "PerfStat.h"
#include <cstdio>
#include <string>
#include <map>

// Static class members initialization here!
std::map<std::string,PerfStatHistory,std::less<std::string> > PerfStat::groupHistoryMap;
bool PerfStat::wantDebugOut = false;
timeval PerfStat::firstDumpTime;
bool PerfStat::firstDumpTimeSet = false;

// Constructor handles starting the timer
PerfStat::PerfStat(std::string groupName) {
	this->group = groupName;
	gettimeofday(&this->start,NULL);

	// If this is our first ever PerfStat object, we'll also initialize this
	if (!firstDumpTimeSet) {
		gettimeofday(&firstDumpTime,NULL);
		firstDumpTimeSet=true;
	}
}

// Destructor handles recording all of our stats
PerfStat::~PerfStat() {
	timeval end;
	gettimeofday(&end,NULL);
	double elapsed = ((end.tv_usec-start.tv_usec)/1000000.0)+(end.tv_sec-start.tv_sec);
	
	double average = elapsed;
	double totalTime = elapsed;
	long int count = 1;
	
	// check to see if this group exists in the history...
	if (groupHistoryMap.find(group) == groupHistoryMap.end()) {
		groupHistoryMap[group]=PerfStatHistory(group,elapsed,1);
	} else {
		PerfStatHistory history = groupHistoryMap[group];
		history.recordTime(elapsed);
		groupHistoryMap[group] = history;
		average = history.getAverage();
		count = history.getCount();
		totalTime = history.getTotalTime();
	}

	if (wantDebugOut) {	
		printf("PerfStats: %s elapsed:%f average:%lf count:%ld total:%lf ut:%d us:%d ue:%d t:%ld s:%ld e:%ld\n",
			this->group.c_str(),elapsed,average,count,totalTime,
			(end.tv_usec-start.tv_usec),start.tv_usec,end.tv_usec,
			(end.tv_sec-start.tv_sec),start.tv_sec,end.tv_sec
		);
	}
};

// How many groups have we added?
int PerfStat::getGroupCount() { 
	return groupHistoryMap.size(); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Method:		DumpStats()
// Description:	Generates some lines of debug stats for all the groups of PerfStats you've created.
// Note: 		Caller is responsible for allocating an array of char*'s that is large enough to hold 
//				groupCount + 1. Caller is also responsible for deleting all this memory.
int PerfStat::DumpStats(char** array) {
	// If we haven't yet set a dump time, we'll also initialize this now, but this is unlikely
	if (!firstDumpTimeSet) {
		gettimeofday(&firstDumpTime,NULL);
		firstDumpTimeSet=true;
	}

	timeval now;
	gettimeofday(&now,NULL);
	double elapsed = ((now.tv_usec-firstDumpTime.tv_usec)/1000000.0)+(now.tv_sec-firstDumpTime.tv_sec);
	
	array[0] = new char[MAX_PERFSTAT_DEBUG_LINE_LEN];
	snprintf(array[0],MAX_PERFSTAT_DEBUG_LINE_LEN,"PerfStats:");
	int lineCount=1;
	// For each active performance group
	for (PerfStatMapItr i = groupHistoryMap.begin(); i != groupHistoryMap.end(); i++) {
		float percent = (i->second.getTotalTime()/elapsed) * 100.0;
	
		array[lineCount] = new char[MAX_PERFSTAT_DEBUG_LINE_LEN];
		snprintf(array[lineCount],MAX_PERFSTAT_DEBUG_LINE_LEN,"%s Avg: %lf Num: %ld TTime: %lf (%.2f%%)",
			i->second.group.c_str(),i->second.getAverage(),i->second.getCount(),i->second.getTotalTime(),percent);
		lineCount++;
	}
	return lineCount;
}

