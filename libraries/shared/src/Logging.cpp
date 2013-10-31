//
//  Logging.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>
#include <cstdio>
#include <iostream>
#include <netdb.h>

#include "SharedUtil.h"
#include "NodeList.h"

#include "Logging.h"

sockaddr_in Logging::logstashSocket = {};
char* Logging::targetName = NULL;

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

void Logging::setTargetName(const char* targetName) {
    // remove the old target name, if it exists
    delete Logging::targetName;
    
    // copy over the new target name
    Logging::targetName = new char[strlen(targetName)];
    strcpy(Logging::targetName, targetName);
}

const char* stringForLogType(QtMsgType msgType) {
    switch (msgType) {
        case QtDebugMsg:
            return "DEBUG";
        case QtCriticalMsg:
            return "CRITICAL";
        case QtFatalMsg:
            return "FATAL";
        case QtWarningMsg:
            return "WARNING";
        default:
            return "UNKNOWN";
    }
}

// the following will produce 2000-10-02 13:55:36 -0700
const char DATE_STRING_FORMAT[] = "%F %H:%M:%S %z";

void Logging::verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    // log prefix is in the following format
    // [DEBUG] [TIMESTAMP] [PID:PARENT_PID] [TARGET] logged string
    
    QString prefixString = QString("[%1]").arg(stringForLogType(type));
    
    time_t rawTime;
    time(&rawTime);
    struct tm* localTime = localtime(&rawTime);
    
    char dateString[100];
    strftime(dateString, sizeof(dateString), DATE_STRING_FORMAT, localTime);
    
    prefixString.append(QString(" [%1]").arg(dateString));
    
    prefixString.append(QString(" [%1").arg(getpid()));
    
    pid_t parentProcessID = getppid();
    if (parentProcessID != 0) {
        prefixString.append(QString(":%1]").arg(parentProcessID));
    } else {
        prefixString.append("]");
    }
    
    if (Logging::targetName) {
        prefixString.append(QString(" [%1]").arg(Logging::targetName));
    }
    
    fprintf(stdout, "%s %s", prefixString.toLocal8Bit().constData(), message.toLocal8Bit().constData());
}