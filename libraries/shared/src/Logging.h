//
//  Logging.h
//  hifi
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Logging__
#define __hifi__Logging__

#include <netinet/in.h>

#include <QtCore/QString>

const int LOGSTASH_UDP_PORT = 9500;
const char LOGSTASH_HOSTNAME[] = "graphite.highfidelity.io";

const char STAT_TYPE_TIMER = 't';
const char STAT_TYPE_COUNTER = 'c';
const char STAT_TYPE_GAUGE = 'g';

class Logging {
public:
    
    enum Type {
        Error,
        Warn,
        Debug
    };
    
    static sockaddr* socket();
    static bool shouldSendStats();
    static void stashValue(char statType, const char* key, float value);
    static void standardizedLog(const QString& output, const char* targetName, Logging::Type logType = Logging::Debug);
private:
    static sockaddr_in logstashSocket;
};

#endif /* defined(__hifi__Logstash__) */
