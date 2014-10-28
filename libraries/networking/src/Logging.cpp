//
//  Logging.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#define getppid _getpid // hack to build
#define pid_t int // hack to build
#endif

#include <qtimer.h>

#include "Logging.h"

QString Logging::_targetName = QString();

Logging& Logging::getInstance() {
    static Logging staticInstance;
    return staticInstance;
}

Logging::Logging() {
    // setup our timer to flush the verbose logs every 5 seconds
    QTimer* logFlushTimer = new QTimer(this);
    connect(logFlushTimer, &QTimer::timeout, this, &Logging::flushRepeatedMessages);
    logFlushTimer->start(VERBOSE_LOG_INTERVAL_SECONDS * 1000);
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

    if (!getInstance()._targetName.isEmpty()) {
        prefixString.append(QString(" [%1]").arg(getInstance()._targetName));
    }
    
    fprintf(stdout, "%s %s\n", prefixString.toLocal8Bit().constData(), message.toLocal8Bit().constData());
}

void Logging::flushRepeatedMessages() {
    QHash<QString, int>::iterator message = _repeatMessageCountHash.begin();
    while (message != _repeatMessageCountHash.end()) {
        message = _repeatMessageCountHash.erase(message);
    }
}
