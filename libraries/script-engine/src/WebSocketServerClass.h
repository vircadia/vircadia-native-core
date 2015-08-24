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
#include "WebSocketClass.h"

class WebSocketServerClass : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString url READ getURL)
    Q_PROPERTY(quint16 port READ getPort)
    Q_PROPERTY(bool listening READ isListening)

public:
    WebSocketServerClass(QScriptEngine* engine, const QString& serverName, const quint16 port);
    ~WebSocketServerClass();

    QString getURL() { return _webSocketServer.serverUrl().toDisplayString(); }
    quint16 getPort() { return _webSocketServer.serverPort(); }
    bool isListening() { return _webSocketServer.isListening(); }

    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

public slots:
    void close();

private:
    QWebSocketServer _webSocketServer;
    QScriptEngine* _engine;
    QList<WebSocketClass*> _clients;

private slots:
    void onNewConnection();

signals:
    void newConnection(WebSocketClass* client);

};

#endif // hifi_WebSocketServerClass_h
