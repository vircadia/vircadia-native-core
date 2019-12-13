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

#include "WebSocketClass.h"

#include "ScriptEngine.h"

WebSocketClass::WebSocketClass(QScriptEngine* engine, QString url) :
    _webSocket(new QWebSocket()),
    _engine(engine)
{
    initialize();
    _webSocket->open(url);
}

WebSocketClass::WebSocketClass(QScriptEngine* engine, QWebSocket* qWebSocket) :
    _webSocket(qWebSocket),
    _engine(engine)
{
    initialize();
}

void WebSocketClass::initialize() {
    connect(_webSocket, &QWebSocket::disconnected, this, &WebSocketClass::handleOnClose);
    connect(_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClass::handleOnMessage);
    connect(_webSocket, &QWebSocket::connected, this, &WebSocketClass::handleOnOpen);
    connect(_webSocket, static_cast<void(QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error), this,
        &WebSocketClass::handleOnError);
    _binaryType = QStringLiteral("blob");
}

QScriptValue WebSocketClass::constructor(QScriptContext* context, QScriptEngine* engine) {
    QString url;
    if (context->argumentCount() > 0) {
        url = context->argument(0).toString();
    }
    return engine->newQObject(new WebSocketClass(engine, url), QScriptEngine::ScriptOwnership);
}

WebSocketClass::~WebSocketClass() {
    _webSocket->deleteLater();
}

void WebSocketClass::send(QScriptValue message) {
    _webSocket->sendTextMessage(message.toString());
}

void WebSocketClass::close() {
    this->close(QWebSocketProtocol::CloseCodeNormal);
}

void WebSocketClass::close(QWebSocketProtocol::CloseCode closeCode) {
    this->close(closeCode, QStringLiteral(""));
}

void WebSocketClass::close(QWebSocketProtocol::CloseCode closeCode, QString reason) {
    _webSocket->close(closeCode, reason);
}

/**jsdoc
 * Called when the connection closes.
 * @callback WebSocket~onCloseCallback
 * @param {WebSocket.CloseData} data - Information on the connection closure.
 */
/**jsdoc
 * Information on a connection being closed.
 * @typedef {object} WebSocket.CloseData
 * @property {WebSocket.CloseCode} code - The reason why the connection was closed.
 * @property {string} reason - Description of the reason why the connection was closed.
 * @property {boolean} wasClean - <code>true</code> if the connection closed cleanly, <code>false</code> if it didn't.
 */
void WebSocketClass::handleOnClose() {
    bool hasError = (_webSocket->error() != QAbstractSocket::UnknownSocketError);
    if (_onCloseEvent.isFunction()) {
        QScriptValueList args;
        QScriptValue arg = _engine->newObject();
        arg.setProperty("code", hasError ? QWebSocketProtocol::CloseCodeAbnormalDisconnection : _webSocket->closeCode());
        arg.setProperty("reason", _webSocket->closeReason());
        arg.setProperty("wasClean", !hasError);
        args << arg;
        _onCloseEvent.call(QScriptValue(), args);
    }
}

/**jsdoc
 * Called when an error occurs.
 * @callback WebSocket~onErrorCallback
 */
void WebSocketClass::handleOnError(QAbstractSocket::SocketError error) {
    if (_onErrorEvent.isFunction()) {
        _onErrorEvent.call();
    }
}

/**jsdoc
 * Triggered when a message is received.
 * @callback WebSocket~onMessageCallback
 * @param {WebSocket.MessageData} message - The message received.
 */
/**jsdoc
 * A message received on a WebSocket connection.
 * @typedef {object} WebSocket.MessageData
 * @property {string} data - The message content.
 */
void WebSocketClass::handleOnMessage(const QString& message) {
    if (_onMessageEvent.isFunction()) {
        QScriptValueList args;
        QScriptValue arg = _engine->newObject();
        arg.setProperty("data", message);
        args << arg;
        _onMessageEvent.call(QScriptValue(), args);
    }
}

/**jsdoc
 * Called when the connection opens.
 * @callback WebSocket~onOpenCallback
 */
void WebSocketClass::handleOnOpen() {
    if (_onOpenEvent.isFunction()) {
        _onOpenEvent.call();
    }
}

QScriptValue qWSCloseCodeToScriptValue(QScriptEngine* engine, const QWebSocketProtocol::CloseCode &closeCode) {
    return closeCode;
}

void qWSCloseCodeFromScriptValue(const QScriptValue &object, QWebSocketProtocol::CloseCode &closeCode) {
    closeCode = (QWebSocketProtocol::CloseCode)object.toUInt16();
}

QScriptValue webSocketToScriptValue(QScriptEngine* engine, WebSocketClass* const &in) {
    return engine->newQObject(in, QScriptEngine::ScriptOwnership);
}

void webSocketFromScriptValue(const QScriptValue &object, WebSocketClass* &out) {
    out = qobject_cast<WebSocketClass*>(object.toQObject());
}

QScriptValue wscReadyStateToScriptValue(QScriptEngine* engine, const WebSocketClass::ReadyState& readyState) {
    return readyState;
}

void wscReadyStateFromScriptValue(const QScriptValue& object, WebSocketClass::ReadyState& readyState) {
    readyState = (WebSocketClass::ReadyState)object.toUInt16();
}
