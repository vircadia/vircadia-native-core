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

#include <qcoreapplication.h>

#include <QDateTime>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QMutexLocker>
#include <QRegExp>

#include "LogHandler.h"

LogHandler& LogHandler::getInstance() {
    static LogHandler staticInstance;
    return staticInstance;
}

LogHandler::LogHandler()
{
    // setup our timer to flush the verbose logs every 5 seconds
    QTimer* logFlushTimer = new QTimer(this);
    connect(logFlushTimer, &QTimer::timeout, this, &LogHandler::flushRepeatedMessages);
    logFlushTimer->start(VERBOSE_LOG_INTERVAL_SECONDS * 1000);

    // when the log handler is first setup we should print our timezone
    QString timezoneString = "Time zone: " + QDateTime::currentDateTime().toString("t");
    printf("%s\n", qPrintable(timezoneString));
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

void LogHandler::flushRepeatedMessages() {
    QMutexLocker locker(&_repeatedMessageLock);
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
        QMutexLocker locker(&_repeatedMessageLock);
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
    if (type == LogDebug) {
        QMutexLocker locker(&_onlyOnceMessageLock);
        // see if this message is one we should only print once
        foreach(const QString& regexString, getInstance()._onlyOnceMessageRegexes) {
            QRegExp onlyOnceRegex(regexString);
            if (onlyOnceRegex.indexIn(message) != -1) {
                if (!_onlyOnceMessageCountHash.contains(message)) {
                    // we have a match and haven't yet printed this message.
                    _onlyOnceMessageCountHash[message] = 1;
                    // break the foreach so we output the first match
                    break;
                } else {
                    // We've already printed this message, don't print it again.
                    return QString();
                }
            }
        }
    }

    // log prefix is in the following format
    // [TIMESTAMP] [DEBUG] [PID] [TID] [TARGET] logged string

    const QString* dateFormatPtr = &DATE_STRING_FORMAT;
    if (_shouldDisplayMilliseconds) {
        dateFormatPtr = &DATE_STRING_FORMAT_WITH_MILLISECONDS;
    }

    QString prefixString = QString("[%1] [%2] [%3]").arg(QDateTime::currentDateTime().toString(*dateFormatPtr),
        stringForLogType(type), context.category);

    if (_shouldOutputProcessID) {
        prefixString.append(QString(" [%1]").arg(QCoreApplication::instance()->applicationPid()));
    }

    if (_shouldOutputThreadID) {
        size_t threadID = (size_t)QThread::currentThreadId();
        prefixString.append(QString(" [%1]").arg(threadID));
    }

    if (!_targetName.isEmpty()) {
        prefixString.append(QString(" [%1]").arg(_targetName));
    }

    QString logMessage = QString("%1 %2").arg(prefixString, message.split("\n").join("\n" + prefixString + " "));
    fprintf(stdout, "%s\n", qPrintable(logMessage));
    return logMessage;
}

void LogHandler::verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    getInstance().printMessage((LogMsgType) type, context, message);
}

const QString& LogHandler::addRepeatedMessageRegex(const QString& regexString) {
    QMutexLocker locker(&_repeatedMessageLock);
    return *_repeatedMessageRegexes.insert(regexString);
}

const QString& LogHandler::addOnlyOnceMessageRegex(const QString& regexString) {
    QMutexLocker locker(&_onlyOnceMessageLock);
    return *_onlyOnceMessageRegexes.insert(regexString);
}
