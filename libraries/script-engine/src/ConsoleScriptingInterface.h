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
#include <QMap>


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
    void asserts(bool condition, QString data="");
    void trace(QString labelName);
    void clear();
public:
    QString secondsToString(qint64 seconds);
    bool isInteger(const std::string & s);
private:
    QMap<QString, QDateTime> _listOfTimeValues;
    const qint64 DAY = 86400;
    const QString TIME_FORMAT = "yyyy-MM-dd HH:mm";
};

#endif // hifi_ConsoleScriptingInterface_h
