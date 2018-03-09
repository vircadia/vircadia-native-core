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

QMutex LogHandler::_mutex;

LogHandler& LogHandler::getInstance() {
    static LogHandler staticInstance;
    return staticInstance;
}

LogHandler::LogHandler() {
    // when the log handler is first setup we should print our timezone
    QString timezoneString = "Time zone: " + QDateTime::currentDateTime().toString("t");
    printMessage(LogMsgType::LogInfo, QMessageLogContext(), timezoneString);
}

LogHandler::~LogHandler() {
    flushRepeatedMessages();
    printMessage(LogMsgType::LogDebug, QMessageLogContext(), "LogHandler shutdown.");
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
    for(auto& message: _repeatedMessages) {

        if (message.messageCount > 1) {
            QString repeatMessage = QString("%1 repeated log entries matching \"%2\" - Last entry: \"%3\"")
                .arg(message.messageCount - 1)
                .arg(message.regexp.pattern())
                .arg(message.lastMessage);

            QMessageLogContext emptyContext;
            lock.unlock();
            printMessage(LogSuppressed, emptyContext, repeatMessage);
            lock.relock();
        }

        message.messageCount = 0;
    }
}

QString LogHandler::printMessage(LogMsgType type, const QMessageLogContext& context, const QString& message) {
    QMutexLocker lock(&_mutex);
    if (message.isEmpty()) {
        return QString();
    }

    if (type == LogDebug) {
        // for debug messages, check if this matches any of our regexes for repeated log messages
        for (auto& repeatRegex : _repeatedMessages) {
            if (repeatRegex.regexp.indexIn(message) != -1) {
                // If we've printed the first one then return out.
                if (repeatRegex.messageCount++ == 0) {
                    break;
                }
                repeatRegex.lastMessage = message;
                return QString();
            }
        }
    }

    if (type == LogDebug) {
        // see if this message is one we should only print once
        for (auto& onceOnly : _onetimeMessages) {
            if (onceOnly.regexp.indexIn(message) != -1) {
                if (onceOnly.messageCount++ == 0) {
                    // we have a match and haven't yet printed this message.
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

const QString& LogHandler::addRepeatedMessageRegex(const QString& regexString) {
    // make sure we setup the repeated message flusher, but do it on the LogHandler thread
    QMetaObject::invokeMethod(this, "setupRepeatedMessageFlusher");

    QMutexLocker lock(&_mutex);
    RepeatedMessage repeatRecord;
    repeatRecord.regexp = QRegExp(regexString);
    _repeatedMessages.push_back(repeatRecord);
    return regexString;
}

const QString& LogHandler::addOnlyOnceMessageRegex(const QString& regexString) {
    QMutexLocker lock(&_mutex);
    OnceOnlyMessage onetimeMessage;
    onetimeMessage.regexp = QRegExp(regexString);
    _onetimeMessages.push_back(onetimeMessage);
    return regexString;
}
