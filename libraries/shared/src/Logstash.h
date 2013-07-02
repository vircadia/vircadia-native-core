//
//  Logstash.h
//  hifi
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Logstash__
#define __hifi__Logstash__

#include <netinet/in.h>

const int LOGSTASH_UDP_PORT = 9500;
const char LOGSTASH_HOSTNAME[] = "graphite.highfidelity.io";

class Logstash {
public:
    static sockaddr* socket();
    static bool shouldSendStats();
    static void stashTimerValue(const char* key, float value);
    static void stashCounterValue(const char* key, float value);
    static void stashGaugeValue(const char* key, float value);
private:
    static void stashValue(char valueType, const char* key, float value);
    
    static sockaddr_in logstashSocket;
};

#endif /* defined(__hifi__Logstash__) */
