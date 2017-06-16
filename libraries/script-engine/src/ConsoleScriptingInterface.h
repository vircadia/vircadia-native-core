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
public:
    static QScriptValue info(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue log(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue debug(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue warn(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue error(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue exception(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue assertion(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue group(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue groupCollapsed(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue groupEnd(QScriptContext* context, QScriptEngine* engine);

public slots:
    void time(QString labelName);
    void timeEnd(QString labelName);
    void trace();
    void clear();

private:    
    QHash<QString, QDateTime> _timerDetails;
    static QList<QString> _groupDetails;
    static void logGroupMessage(QString message, QScriptEngine* engine);
    static QString appendArguments(QScriptContext* context);
};

#endif // hifi_ConsoleScriptingInterface_h
