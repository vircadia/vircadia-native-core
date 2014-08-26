//
//  LocationScriptingInterface.h
//  interface/src/scripting
//
//  Created by Ryan Huffman on 4/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LocationScriptingInterface_h
#define hifi_LocationScriptingInterface_h

#include <QObject>
#include <QScriptContext>
#include <QScriptEngine>
#include <QScriptValue>
#include <QString>

#include "Application.h"

class LocationScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isConnected READ isConnected)
    Q_PROPERTY(QString href READ getHref)
    Q_PROPERTY(QString protocol READ getProtocol)
    Q_PROPERTY(QString hostname READ getHostname)
    Q_PROPERTY(QString pathname READ getPathname)
    LocationScriptingInterface() { };
public:
    static LocationScriptingInterface* getInstance();

    bool isConnected();
    QString getHref();
    QString getProtocol() { return CUSTOM_URL_SCHEME; };
    QString getPathname();
    QString getHostname();

    static QScriptValue locationGetter(QScriptContext* context, QScriptEngine* engine);
    static QScriptValue locationSetter(QScriptContext* context, QScriptEngine* engine);

public slots:
    void assign(const QString& url);

};

#endif // hifi_LocationScriptingInterface_h
