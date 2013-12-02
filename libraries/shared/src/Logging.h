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

class HifiSockAddr;

/// Handles custom message handling and sending of stats/logs to Logstash instance
class Logging {
public:
    /// \return the socket used to send stats to logstash
    static const HifiSockAddr& socket();
    
    /// checks if this target should send stats to logstash, given its current environment
    /// \return true if the caller should send stats to logstash
    static bool shouldSendStats();
    
    /// stashes a float value to Logstash instance
    /// \param statType a stat type from the constants in this file
    /// \param key the key at which to store the stat
    /// \param value the value to store
    static void stashValue(char statType, const char* key, float value);
    
    /// sets the target name to output via the verboseMessageHandler, called once before logging begins
    /// \param targetName the desired target name to output in logs
    static void setTargetName(const char* targetName);
    
    /// a qtMessageHandler that can be hooked up to a target that links to Qt
    /// prints various process, message type, and time information
    static void verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message);
private:
    static HifiSockAddr logstashSocket;
    static char* targetName;
};

#endif /* defined(__hifi__Logstash__) */
