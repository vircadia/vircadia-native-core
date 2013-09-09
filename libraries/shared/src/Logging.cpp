//
//  Logging.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>
#include <cstdio>
#include <netdb.h>

#include "SharedUtil.h"
#include "NodeList.h"

#include "Logging.h"

sockaddr_in Logging::logstashSocket = {};

sockaddr* Logging::socket() {
    
    if (logstashSocket.sin_addr.s_addr == 0) {
        // we need to construct the socket object
        
        // assume IPv4
        logstashSocket.sin_family = AF_INET;
        
        // use the constant port
        logstashSocket.sin_port = htons(LOGSTASH_UDP_PORT);
        
        // lookup the IP address for the constant hostname
        struct hostent* logstashHostInfo;
        if ((logstashHostInfo = gethostbyname(LOGSTASH_HOSTNAME))) {
            memcpy(&logstashSocket.sin_addr, logstashHostInfo->h_addr_list[0], logstashHostInfo->h_length);
        } else {
            printf("Failed to lookup logstash IP - will try again on next log attempt.\n");
        }
    }
    
    return (sockaddr*) &logstashSocket;
}

bool Logging::shouldSendStats() {
    static bool shouldSendStats = isInEnvironment("production");
    return shouldSendStats;
}

void Logging::stashValue(char statType, const char* key, float value) {
    static char logstashPacket[MAX_PACKET_SIZE];
    
    // load up the logstash packet with the key and the passed float value
    // send it to 4 decimal places
    int numPacketBytes = sprintf(logstashPacket, "%c %s %.4f", statType, key, value);
    
    NodeList *nodeList = NodeList::getInstance();
    
    if (nodeList) {
        nodeList->getNodeSocket()->send(socket(), logstashPacket, numPacketBytes);
    }
}

const QString DEBUG_STRING = "DEBUG";
const QString WARN_STRING = "WARN";
const QString ERROR_STRING = "ERROR";

const QString& stringForLogType(Logging::Type logType) {
    if (logType == Logging::Debug) {
        return DEBUG_STRING;
    } else if (logType == Logging::Warn) {
        return WARN_STRING;
    } else {
        return ERROR_STRING;
    }
}

// the following will produce 2000-10-02 13:55:36 -0700
const char DATE_STRING_FORMAT[] = "%F %H:%M:%S %z";

void Logging::standardizedLog(const QString &output, const char* targetName, Logging::Type logType) {
    time_t rawTime;
    time(&rawTime);
    struct tm* localTime = localtime(&rawTime);
    
    // log prefix is in the following format
    // [DEBUG] [TIMESTAMP] [PID:PARENT_PID] [TARGET] logged string
    
    QString prefixString = QString("[%1] ").arg(stringForLogType(logType));
    
    char dateString[100];
    strftime(dateString, sizeof(dateString), DATE_STRING_FORMAT, localTime);
    
    prefixString.append(QString("[%1] ").arg(dateString));
    
    prefixString.append(QString("[%1").arg(getpid()));
    
    pid_t parentProcessID = getppid();
    if (parentProcessID != 0) {
        prefixString.append(QString(":%1] ").arg(parentProcessID));
    } else {
        prefixString.append("]");
    }
    
    prefixString.append(QString("[%1]").arg(targetName));
    
    qDebug("%s %s", prefixString.toStdString().c_str(), output.toStdString().c_str());
}