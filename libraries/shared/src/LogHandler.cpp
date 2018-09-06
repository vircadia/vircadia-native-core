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

#include "LogHandler.h"

#include <mutex>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>
#include <QtCore/QThread>
#include <QtCore/QTimer>

QMutex LogHandler::_mutex(QMutex::Recursive);

LogHandler& LogHandler::getInstance() {
    static LogHandler staticInstance;
    return staticInstance;
}

LogHandler::LogHandler() {
    // make sure we setup the repeated message flusher, but do it on the LogHandler thread	
    QMetaObject::invokeMethod(this, "setupRepeatedMessageFlusher");
}

LogHandler::~LogHandler() {
    flushRepeatedMessages();
}

const char* stringForLogType(LogMsgType msgType) {
    switch (msgType) {
        case LogInfo:
            return "INFO";
        case LogDebug:
            return "DEBUG";
        case LogWarning:
            return "WARNING";
        case LogCritical:
            return "CRITICAL";
        case LogFatal:
            return "FATAL";
        case LogSuppressed:
            return "SUPPRESS";
        default:
            return "UNKNOWN";
    }
}

// the following will produce 11/18 13:55:36
const QString DATE_STRING_FORMAT = "MM/dd hh:mm:ss";

// the following will produce 11/18 13:55:36.999
const QString DATE_STRING_FORMAT_WITH_MILLISECONDS = "MM/dd hh:mm:ss.zzz";

void LogHandler::setTargetName(const QString& targetName) {
    QMutexLocker lock(&_mutex);
    _targetName = targetName;
}

void LogHandler::setShouldOutputProcessID(bool shouldOutputProcessID) {
    QMutexLocker lock(&_mutex);
    _shouldOutputProcessID = shouldOutputProcessID;
}

void LogHandler::setShouldOutputThreadID(bool shouldOutputThreadID) {
    QMutexLocker lock(&_mutex);
    _shouldOutputThreadID = shouldOutputThreadID;
}

void LogHandler::setShouldDisplayMilliseconds(bool shouldDisplayMilliseconds) {
    QMutexLocker lock(&_mutex);
    _shouldDisplayMilliseconds = shouldDisplayMilliseconds;
}


void LogHandler::flushRepeatedMessages() {
    QMutexLocker lock(&_mutex);

    // New repeat-suppress scheme:
    for (int m = 0; m < (int)_repeatedMessageRecords.size(); ++m) {
        int repeatCount = _repeatedMessageRecords[m].repeatCount;
        if (repeatCount > 1) {
            QString repeatLogMessage = QString().setNum(repeatCount) + " repeated log entries - Last entry: \"" 
                    + _repeatedMessageRecords[m].repeatString + "\"";
            printMessage(LogSuppressed, QMessageLogContext(), repeatLogMessage);
            _repeatedMessageRecords[m].repeatCount = 0;
            _repeatedMessageRecords[m].repeatString = QString();
        }
    }
}

QString LogHandler::printMessage(LogMsgType type, const QMessageLogContext& context, const QString& message) {
    if (message.isEmpty()) {
        return QString();
    }
    QMutexLocker lock(&_mutex);

    // log prefix is in the following format
    // [TIMESTAMP] [DEBUG] [PID] [TID] [TARGET] logged string

    const QString* dateFormatPtr = &DATE_STRING_FORMAT;
    if (_shouldDisplayMilliseconds) {
        dateFormatPtr = &DATE_STRING_FORMAT_WITH_MILLISECONDS;
    }

    QString prefixString = QString("[%1] [%2] [%3]").arg(QDateTime::currentDateTime().toString(*dateFormatPtr),
        stringForLogType(type), context.category);

    if (_shouldOutputProcessID) {
        prefixString.append(QString(" [%1]").arg(QCoreApplication::applicationPid()));
    }

    if (_shouldOutputThreadID) {
        size_t threadID = (size_t)QThread::currentThreadId();
        prefixString.append(QString(" [%1]").arg(threadID));
    }

    if (!_targetName.isEmpty()) {
        prefixString.append(QString(" [%1]").arg(_targetName));
    }

    // for [qml] console.* messages include an abbreviated source filename
    if (context.category && context.file && !strcmp("qml", context.category)) {
        if (const char* basename = strrchr(context.file, '/')) {
            prefixString.append(QString(" [%1]").arg(basename+1));
        }
    }

    QString logMessage = QString("%1 %2\n").arg(prefixString, message.split('\n').join('\n' + prefixString + " "));

    fprintf(stdout, "%s", qPrintable(logMessage));
#ifdef Q_OS_WIN
    // On windows, this will output log lines into the Visual Studio "output" tab
    OutputDebugStringA(qPrintable(logMessage));
#endif
    return logMessage;
}

void LogHandler::verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    getInstance().printMessage((LogMsgType) type, context, message);
}

void LogHandler::setupRepeatedMessageFlusher() {
    static std::once_flag once;
    std::call_once(once, [&] {
        // setup our timer to flush the verbose logs every 5 seconds
        QTimer* logFlushTimer = new QTimer(this);
        connect(logFlushTimer, &QTimer::timeout, this, &LogHandler::flushRepeatedMessages);
        logFlushTimer->start(VERBOSE_LOG_INTERVAL_SECONDS * 1000);
    });
}

int LogHandler::newRepeatedMessageID() {
    QMutexLocker lock(&_mutex);
    int newMessageId = _currentMessageID;
    ++_currentMessageID;
    RepeatedMessageRecord newRecord { 0, QString() };
    _repeatedMessageRecords.push_back(newRecord);
    return newMessageId;
}

void LogHandler::printRepeatedMessage(int messageID, LogMsgType type, const QMessageLogContext& context,
                                      const QString& message) {
    QMutexLocker lock(&_mutex);
    if (messageID >= _currentMessageID) {
        return;
    }

    if (_repeatedMessageRecords[messageID].repeatCount == 0) {
        printMessage(type, context, message);
    } else {
        _repeatedMessageRecords[messageID].repeatString = message;
    }
 
    ++_repeatedMessageRecords[messageID].repeatCount;
}
