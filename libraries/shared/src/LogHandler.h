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

#include <qhash.h>
#include <qobject.h>
#include <qregexp.h>
#include <qset.h>
#include <qstring.h>

const int VERBOSE_LOG_INTERVAL_SECONDS = 5;

/// Handles custom message handling and sending of stats/logs to Logstash instance
class LogHandler : public QObject {
    Q_OBJECT
public:
    static LogHandler& getInstance();
    
    /// sets the target name to output via the verboseMessageHandler, called once before logging begins
    /// \param targetName the desired target name to output in logs
    void setTargetName(const QString& targetName) { _targetName = targetName; }
    
    void setShouldOutputPID(bool shouldOutputPID) { _shouldOutputPID = shouldOutputPID; }
    
    QString printMessage(QtMsgType type, const QMessageLogContext& context, const QString &message);
    
    /// a qtMessageHandler that can be hooked up to a target that links to Qt
    /// prints various process, message type, and time information
    static void verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message);
    
    const QString& addRepeatedMessageRegex(const QString& regexString) { return *_repeatedMessageRegexes.insert(regexString); }
private:
    LogHandler();
    
    void flushRepeatedMessages();
    
    QString _targetName;
    bool _shouldOutputPID;
    QSet<QString> _repeatedMessageRegexes;
    QHash<QString, int> _repeatMessageCountHash;
    QHash<QString, QString> _lastRepeatedMessage;
};

#endif // hifi_LogHandler_h