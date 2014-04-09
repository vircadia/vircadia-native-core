//
//  Logging.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cstring>
#include <cstdio>
#include <iostream>
#include <ctime>
//#include <netdb.h> // not available on windows, apparently not needed on mac

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#define getppid _getpid // hack to build
#define pid_t int // hack to build
#endif

#include <QtNetwork/QHostInfo>

#include "HifiSockAddr.h"
#include "SharedUtil.h"
#include "NodeList.h"

#include "Logging.h"

HifiSockAddr Logging::_logstashSocket = HifiSockAddr();
QString Logging::_targetName = QString();

const HifiSockAddr& Logging::socket() {

    if (_logstashSocket.getAddress().isNull()) {
        // we need to construct the socket object
        // use the constant port
        _logstashSocket.setPort(htons(LOGSTASH_UDP_PORT));

        // lookup the IP address for the constant hostname
        QHostInfo hostInfo = QHostInfo::fromName(LOGSTASH_HOSTNAME);
        if (!hostInfo.addresses().isEmpty()) {
            // use the first IP address
            _logstashSocket.setAddress(hostInfo.addresses().first());
        } else {
            printf("Failed to lookup logstash IP - will try again on next log attempt.\n");
        }
    }

    return _logstashSocket;
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
        nodeList->getNodeSocket().writeDatagram(logstashPacket, numPacketBytes,
                                                _logstashSocket.getAddress(), _logstashSocket.getPort());
    }
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
const char DATE_STRING_FORMAT[] = "%Y-%m-%d %H:%M:%S %z";

void Logging::verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (message.isEmpty()) {
        return;
    }
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

    if (!_targetName.isEmpty()) {
        prefixString.append(QString(" [%1]").arg(_targetName));
    }
    
    fprintf(stdout, "%s %s\n", prefixString.toLocal8Bit().constData(), message.toLocal8Bit().constData());
}
