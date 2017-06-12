//
//  ConsoleScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by volansystech on 6/1/17.
//  Copyright 2014 High Fidelity, Inc.
//
//  ConsoleScriptingInterface is responsible to print log.
//  using "console" object on debug Window and Logs/log file with proper tags.
//  Used in Javascripts file.
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
#include <QHash>
#include "EntityEditFilters.h"
#include <QUrl>
#include <QList>


/// Scriptable interface of console object. Used exclusively in the JavaScript API
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
    void consoleGroupLog(QString currentGroup, QString groupName);
public:
    QString secondsToString(qint64 seconds);    
    bool hasCorrectSyntax(QScriptProgram& program);
    QString hadUncaughtExceptions(QScriptEngine& engine, QString& fileName);
private:
    QHash<QString, QDateTime> _listOfTimeValues;
    const qint64 DAY = 86400;
    const QString TIME_FORMAT = "yyyy-MM-dd HH:mm";       
    QString _selectedGroup;
    const QString GROUP = "group";
    const QString GROUPCOLLAPSED = "groupCollapse";
    const QString GROUPEND = "groupEnd";
    bool _isSameLevel = false;
    QString _addSpace = "";
    bool _isGroupEnd = true;    
};

#endif // hifi_ConsoleScriptingInterface_h
