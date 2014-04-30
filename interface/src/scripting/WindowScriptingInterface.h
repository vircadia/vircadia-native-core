//
//  WindowScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WindowScriptingInterface_h
#define hifi_WindowScriptingInterface_h

#include <QDebug>
#include <QMutex>
#include <QObject>
#include <QString>

#include "LocationScriptingInterface.h"

class WindowScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(int innerWidth READ getInnerWidth)
    Q_PROPERTY(int innerHeight READ getInnerHeight)
    WindowScriptingInterface() { };
public:
    static WindowScriptingInterface* getInstance();
    int getInnerWidth();
    int getInnerHeight();

public slots:
    QScriptValue alert(const QString& message);
    QScriptValue confirm(const QString& message);
    QScriptValue prompt(const QString& message, const QString& defaultText="");

private slots:
    QScriptValue showAlert(const QString& message);
    QScriptValue showConfirm(const QString& message);
    QScriptValue showPrompt(const QString& message, const QString& defaultText);
};

#endif // hifi_WindowScriptingInterface_h
