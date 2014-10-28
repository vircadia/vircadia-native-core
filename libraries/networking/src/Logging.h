//
//  Logging.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Logging_h
#define hifi_Logging_h

#include <qhash.h>
#include <qobject.h>
#include <qregexp.h>
#include <qset.h>
#include <qstring.h>

const int VERBOSE_LOG_INTERVAL_SECONDS = 5;

/// Handles custom message handling and sending of stats/logs to Logstash instance
class Logging : public QObject {
    Q_OBJECT
public:
    /// sets the target name to output via the verboseMessageHandler, called once before logging begins
    /// \param targetName the desired target name to output in logs
    static void setTargetName(const QString& targetName) { _targetName = targetName; }

    /// a qtMessageHandler that can be hooked up to a target that links to Qt
    /// prints various process, message type, and time information
    static void verboseMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString &message);
    
    static void addRepeatedMessageRegex(const QRegExp& regex) { getInstance()._repeatedMessageRegexes.append(regex); }
private:
    static Logging& getInstance();
    Logging();
    
    void flushRepeatedMessages();
    
    static QString _targetName;
    QList<QRegExp> _repeatedMessageRegexes;
    QHash<QString, int> _repeatMessageCountHash;
};

#endif // hifi_Logging_h
