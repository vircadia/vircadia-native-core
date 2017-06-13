//
//  ConsoleScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by NeetBhagat on 6/1/17.
//  Copyright 2014 High Fidelity, Inc.
//
//  ConsoleScriptingInterface is responsible for following functionality
//  Printing logs with various tags and grouping on debug Window and Logs/log file.
//  Debugging functionalities like Timer start-end, assertion, trace.
//  To use these functionalities, use "console" object in Javascript files.
//  For examples please refer "scripts/developer/tests/consoleObjectTest.js"
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_ConsoleScriptingInterface_h
#define hifi_ConsoleScriptingInterface_h

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtScript/QScriptable>
#include <QList>
#include <QHash>

// Scriptable interface of "console" object. Used exclusively in the JavaScript API
class ConsoleScriptingInterface : public QObject, protected QScriptable {
    Q_OBJECT
public slots:
    void info(QString message);
    void log(QString message);
    void debug(QString message);
    void warn(QString message);
    void error(QString message);
    void exception(QString message);
    void time(QString labelName);
    void timeEnd(QString labelName);
    void asserts(bool condition, QString message = "");
    void trace();
    void clear();
    void group(QString groupName);
    void groupCollapsed(QString groupName);
    void groupEnd();
private:
    QHash<QString, QDateTime> _timerDetails;
    QList<QString> _groupDetails;
    void logGroupMessage(QString msg);
};

#endif // hifi_ConsoleScriptingInterface_h
