//
//  LogHandler.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2014-10-28.
//  Migrated from Logging.cpp created on 6/11/13
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <time.h>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#define getppid _getpid // hack to build
#define pid_t int // hack to build
#elif __linux__
#include <unistd.h> // for getpid() on linux
#endif

#include <qdebug.h>
#include <qtimer.h>

#include "LogHandler.h"

LogHandler& LogHandler::getInstance() {
    static LogHandler staticInstance;
    return staticInstance;
}

LogHandler::LogHandler() :
    _shouldOutputPID(false)
{
    // setup our timer to flush the verbose logs every 5 seconds
    QTimer* logFlushTimer = new QTimer(this);
    connect(logFlushTimer, &QTimer::timeout, this, &LogHandler::flushRepeatedMessages);
    logFlushTimer->start(VERBOSE_LOG_INTERVAL_SECONDS * 1000);
}

const char* stringForLogType(LogMsgType msgType) {
    switch (msgType) {
        case QtDebugMsg:
            return "DEBUG";
        case QtCriticalMsg:
            return "CRITICAL";
        case QtFatalMsg:
            return "FATAL";
        case QtWarningMsg:
            return "WARNING";
        case LogSuppressed:
            return "SUPPRESS";
        default:
            return "UNKNOWN";
    }
}

// the following will produce 2000-10-02 13:55:36 -0700
const char DATE_STRING_FORMAT[] = "%Y-%m-%d %H:%M:%S %z";

void LogHandler::flushRepeatedMessages() {
    QHash<QString, int>::iterator message = _repeatMessageCountHash.begin();
    while (message != _repeatMessageCountHash.end()) {
        
        if (message.value() > 0) {
            QString repeatMessage = QString("%1 repeated log entries matching \"%2\" - Last entry: \"%3\"")
            .arg(message.value()).arg(message.key()).arg(_lastRepeatedMessage.value(message.key()));
            
            QMessageLogContext emptyContext;
            printMessage(LogSuppressed, emptyContext, repeatMessage);
        }
        
        _lastRepeatedMessage.remove(message.key());
        message = _repeatMessageCountHash.erase(message);
    }
}

QString LogHandler::printMessage(LogMsgType type, const QMessageLogContext& context, const QString& message) {
    
    if (message.isEmpty()) {
        return QString();
    }
    
    if (type == LogDebug) {
        // for debug messages, check if this matches any of our regexes for repeated log messages
        foreach(const QString& regexString, getInstance()._repeatedMessageRegexes) {
            QRegExp repeatRegex(regexString);
            if (repeatRegex.indexIn(message) != -1) {
                
                if (!_repeatMessageCountHash.contains(regexString)) {
                    // we have a match but didn't have this yet - output the first one
                    _repeatMessageCountHash[regexString] = 0;
                    
                    // break the foreach so we output the first match
                    break;
                } else {
                    // we have a match - add 1 to the count of repeats for this message and set this as the last repeated message
                    _repeatMessageCountHash[regexString] += 1;
                    _lastRepeatedMessage[regexString] = message;
                    
                    // return out, we're not printing this one
                    return QString();
                }
            }
        }
    }
    
    // log prefix is in the following format
    // [DEBUG] [TIMESTAMP] [PID:PARENT_PID] [TARGET] logged string
    
    QString prefixString = QString("[%1]").arg(stringForLogType(type));
    
    time_t rawTime;
    time(&rawTime);
    struct tm* localTime = localtime(&rawTime);
    
    char dateString[100];
    strftime(dateString, sizeof(dateString), DATE_STRING_FORMAT, localTime);
    
    if (_shouldOutputPID) {
        prefixString.append(QString(" [%1").arg(getpid()));
        
        pid_t parentProcessID = getppid();
        if (parentProcessID != 0) {
            prefixString.append(QString(":%1]").arg(parentProcessID));
        } else {
            prefixString.append("]");
        }
    }
    
    if (!_targetName.isEmpty()) {
        prefixString.append(QString(" [%1]").arg(_targetName));
    }
    
    QString logMessage = QString("%1 %2").arg(prefixString, message);
    fprintf(stdout, "%s\n", qPrintable(logMessage));
    return logMessage;
}

void LogHandler::verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    getInstance().printMessage((LogMsgType) type, context, message);
}
