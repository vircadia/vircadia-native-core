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

#ifndef hifi_LogHandler_h
#define hifi_LogHandler_h

#include <QObject>
#include <QString>
#include <QRegExp>
#include <QMutex>
#include <vector>
#include <memory>

const int VERBOSE_LOG_INTERVAL_SECONDS = 5;

enum LogMsgType {
    LogInfo = QtInfoMsg,
    LogDebug = QtDebugMsg,
    LogWarning = QtWarningMsg,
    LogCritical = QtCriticalMsg,
    LogFatal = QtFatalMsg,
    LogSuppressed = 100
};

/// Handles custom message handling and sending of stats/logs to Logstash instance
class LogHandler : public QObject {
    Q_OBJECT
public:
    static LogHandler& getInstance();

    /// sets the target name to output via the verboseMessageHandler, called once before logging begins
    /// \param targetName the desired target name to output in logs
    void setTargetName(const QString& targetName);

    void setShouldOutputProcessID(bool shouldOutputProcessID);
    void setShouldOutputThreadID(bool shouldOutputThreadID);
    void setShouldDisplayMilliseconds(bool shouldDisplayMilliseconds);

    QString printMessage(LogMsgType type, const QMessageLogContext& context, const QString &message);

    /// a qtMessageHandler that can be hooked up to a target that links to Qt
    /// prints various process, message type, and time information
    static void verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message);

    int newRepeatedMessageID();
    void printRepeatedMessage(int messageID, LogMsgType type, const QMessageLogContext& context, const QString &message);

    void setupRepeatedMessageFlusher();

private:
    LogHandler();
    ~LogHandler() = default;

    void flushRepeatedMessages();

    QString _targetName;
    bool _shouldOutputProcessID { false };
    bool _shouldOutputThreadID { false };
    bool _shouldDisplayMilliseconds { false };
    bool _useColor { false };
    bool _keepRepeats { false };

    QString _previousMessage;
    int _repeatCount { 0 };


    int _currentMessageID { 0 };
    struct RepeatedMessageRecord {
        int repeatCount;
        QString repeatString;
    };
    std::vector<RepeatedMessageRecord> _repeatedMessageRecords;
    static QMutex _mutex;
};

#define HIFI_FCDEBUG(category, message) \
    do { \
        if (category.isDebugEnabled()) { \
            static int repeatedMessageID_ = LogHandler::getInstance().newRepeatedMessageID(); \
            QString logString_; \
            QDebug debugStringReceiver_(&logString_); \
            debugStringReceiver_ << message; \
            LogHandler::getInstance().printRepeatedMessage(repeatedMessageID_, LogDebug, QMessageLogContext(__FILE__, \
                __LINE__, __func__, category().categoryName()), logString_); \
        } \
    } while (false)

#define HIFI_FDEBUG(message) HIFI_FCDEBUG((*QLoggingCategory::defaultCategory()), message)
   
#define HIFI_FCDEBUG_ID(category, messageID, message) \
    do { \
        if (category.isDebugEnabled()) { \
            QString logString_; \
            QDebug debugStringReceiver_(&logString_); \
            debugStringReceiver_ << message; \
            LogHandler::getInstance().printRepeatedMessage(messageID, LogDebug, QMessageLogContext(__FILE__, \
                __LINE__, __func__, category().categoryName()), logString_); \
        } \
    } while (false)

#define HIFI_FDEBUG_ID(messageID, message) HIFI_FCDEBUG_ID((*QLoggingCategory::defaultCategory()), messageID, message)

#endif // hifi_LogHandler_h
