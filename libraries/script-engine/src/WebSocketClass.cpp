//
//  WebSocketClass.cpp
//  libraries/script-engine/src/
//
//  Created by Thijs Wenker on 8/4/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  This class is an implementation of the WebSocket object for scripting use.  It provides a near-complete implementation
//  of the class described in the Mozilla docs: https://developer.mozilla.org/en-US/docs/Web/API/WebSocket
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptEngine.h"
#include "WebSocketClass.h"

WebSocketClass::WebSocketClass(QScriptEngine* engine, QString url) :
    _engine(engine)
{
    connect(&_webSocket, &QWebSocket::disconnected, this, &WebSocketClass::handleOnClose);
    connect(&_webSocket, SIGNAL(error()), this, SLOT(handleOnError()));
    connect(&_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClass::handleOnMessage);
    connect(&_webSocket, &QWebSocket::connected, this, &WebSocketClass::handleOnOpen);
    _webSocket.open(url);
}

QScriptValue WebSocketClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    QString url;
    QScriptValue callee = context->callee();
    if (context->argumentCount() == 1) {
        url = context->argument(0).toString();
    }
    return engine->newQObject(new WebSocketClass(engine, url));
}

WebSocketClass::~WebSocketClass() {

}

void WebSocketClass::send(QScriptValue message) {
    _webSocket.sendTextMessage(message.toString());
}

void WebSocketClass::close(QWebSocketProtocol::CloseCode closeCode, QString reason) {
    _webSocket.close(closeCode, reason);
}

void WebSocketClass::handleOnClose() {
    if (_onCloseEvent.isFunction()) {
        //QScriptValueList args;
        //args << ("received: " + message);
        _onCloseEvent.call();//(QScriptValue(), args);
    }
}

void WebSocketClass::handleOnError(QAbstractSocket::SocketError error) {
    if (_onErrorEvent.isFunction()) {
       // QScriptValueList args;
        //args << ("received: " + message);
        _onErrorEvent.call();/// QScriptValue(), args);
    }
}

void WebSocketClass::handleOnMessage(const QString& message) {
    if (_onMessageEvent.isFunction()) {
        QScriptValueList args;
        QScriptValue arg = _engine->newObject();
        arg.setProperty("data", message);
        args << arg;
        _onMessageEvent.call(QScriptValue(), args);
    }
}

void WebSocketClass::handleOnOpen() {
    if (_onOpenEvent.isFunction()) {
        //QScriptValueList args;
        //args << ("received: " + message);
        _onOpenEvent.call();// QScriptValue(), args);
    }
}