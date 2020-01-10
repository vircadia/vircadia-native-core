//
//  WebSocketClass.h
//  libraries/script-engine/src/
//
//  Created by Thijs Wenker on 8/4/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WebSocketClass_h
#define hifi_WebSocketClass_h

#include <QObject>
#include <QScriptEngine>
#include <QWebSocket>

class WebSocketClass : public QObject {
    Q_OBJECT
        Q_PROPERTY(QString binaryType READ getBinaryType WRITE setBinaryType)
        Q_PROPERTY(ulong bufferedAmount READ getBufferedAmount)
        Q_PROPERTY(QString extensions READ getExtensions)

        Q_PROPERTY(QScriptValue onclose READ getOnClose WRITE setOnClose)
        Q_PROPERTY(QScriptValue onerror READ getOnError WRITE setOnError)
        Q_PROPERTY(QScriptValue onmessage READ getOnMessage WRITE setOnMessage)
        Q_PROPERTY(QScriptValue onopen READ getOnOpen WRITE setOnOpen)

        Q_PROPERTY(QString protocol READ getProtocol)
        Q_PROPERTY(WebSocketClass::ReadyState readyState READ getReadyState)
        Q_PROPERTY(QString url READ getURL)

        Q_PROPERTY(WebSocketClass::ReadyState CONNECTING READ getConnecting CONSTANT)
        Q_PROPERTY(WebSocketClass::ReadyState OPEN READ getOpen CONSTANT)
        Q_PROPERTY(WebSocketClass::ReadyState CLOSING READ getClosing CONSTANT)
        Q_PROPERTY(WebSocketClass::ReadyState CLOSED READ getClosed CONSTANT)

public:
    WebSocketClass(QScriptEngine* engine, QString url);
    WebSocketClass(QScriptEngine* engine, QWebSocket* qWebSocket);
    ~WebSocketClass();

    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

    enum ReadyState {
        CONNECTING = 0,
        OPEN,
        CLOSING,
        CLOSED
    };

    QWebSocket* getWebSocket() { return _webSocket; }

    ReadyState getConnecting() const { return CONNECTING; };
    ReadyState getOpen() const { return OPEN; };
    ReadyState getClosing() const { return CLOSING; };
    ReadyState getClosed() const { return CLOSED; };

    void setBinaryType(QString binaryType) { _binaryType = binaryType; }
    QString getBinaryType() { return _binaryType; }

    // extensions is a empty string until supported in QT WebSockets
    QString getExtensions() { return QString(); }

    // protocol is a empty string until supported in QT WebSockets
    QString getProtocol() { return QString(); }

    ulong getBufferedAmount() { return 0; }

    QString getURL() { return _webSocket->requestUrl().toDisplayString(); }

    ReadyState getReadyState() {
        switch (_webSocket->state()) {
            case QAbstractSocket::SocketState::HostLookupState:
            case QAbstractSocket::SocketState::ConnectingState:
                return CONNECTING;
            case QAbstractSocket::SocketState::ConnectedState:
            case QAbstractSocket::SocketState::BoundState:
            case QAbstractSocket::SocketState::ListeningState:
                return OPEN;
            case QAbstractSocket::SocketState::ClosingState:
                return CLOSING;
            case QAbstractSocket::SocketState::UnconnectedState:
            default:
                return CLOSED;
        }
    }

    void setOnClose(QScriptValue eventFunction) { _onCloseEvent = eventFunction; }
    QScriptValue getOnClose() { return _onCloseEvent; }

    void setOnError(QScriptValue eventFunction) { _onErrorEvent = eventFunction; }
    QScriptValue getOnError() { return _onErrorEvent; }

    void setOnMessage(QScriptValue eventFunction) { _onMessageEvent = eventFunction; }
    QScriptValue getOnMessage() { return _onMessageEvent; }

    void setOnOpen(QScriptValue eventFunction) { _onOpenEvent = eventFunction; }
    QScriptValue getOnOpen() { return _onOpenEvent; }

public slots:
    void send(QScriptValue message);

    void close();
    void close(QWebSocketProtocol::CloseCode closeCode);
    void close(QWebSocketProtocol::CloseCode closeCode, QString reason);

private:
    QWebSocket* _webSocket;
    QScriptEngine* _engine;

    QScriptValue _onCloseEvent;
    QScriptValue _onErrorEvent;
    QScriptValue _onMessageEvent;
    QScriptValue _onOpenEvent;

    QString _binaryType;

    void initialize();

private slots:
    void handleOnClose();
    void handleOnError(QAbstractSocket::SocketError error);
    void handleOnMessage(const QString& message);
    void handleOnBinaryMessage(const QByteArray& message);
    void handleOnOpen();

};

Q_DECLARE_METATYPE(QWebSocketProtocol::CloseCode);
Q_DECLARE_METATYPE(WebSocketClass::ReadyState);

QScriptValue qWSCloseCodeToScriptValue(QScriptEngine* engine, const QWebSocketProtocol::CloseCode& closeCode);
void qWSCloseCodeFromScriptValue(const QScriptValue& object, QWebSocketProtocol::CloseCode& closeCode);

QScriptValue webSocketToScriptValue(QScriptEngine* engine, WebSocketClass* const &in);
void webSocketFromScriptValue(const QScriptValue &object, WebSocketClass* &out);

QScriptValue wscReadyStateToScriptValue(QScriptEngine* engine, const WebSocketClass::ReadyState& readyState);
void wscReadyStateFromScriptValue(const QScriptValue& object, WebSocketClass::ReadyState& readyState);

#endif // hifi_WebSocketClass_h
