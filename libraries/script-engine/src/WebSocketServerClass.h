//
//  WebSocketServerClass.h
//  libraries/script-engine/src/
//
//  Created by Thijs Wenker on 8/10/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebSocketServerClass_h
#define hifi_WebSocketServerClass_h

#include <QObject>
#include <QScriptEngine>
#include <QWebSocketServer>

class WebSocketServerClass : public QObject {
    Q_OBJECT

public:
    WebSocketServerClass(QScriptEngine* engine, const QString& serverName, quint16 port);
    ~WebSocketServerClass();

    static QScriptValue WebSocketServerClass::constructor(QScriptContext* context, QScriptEngine* engine);

private:
    QWebSocketServer _webSocketServer;
    QScriptEngine* _engine;

signals:
    void newConnection();



};

#endif // hifi_WebSocketServerClass_h
