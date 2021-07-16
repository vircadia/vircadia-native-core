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

/// @addtogroup ScriptEngine
/// @{

#ifndef hifi_WebSocketClass_h
#define hifi_WebSocketClass_h

#include <QObject>
#include <QScriptEngine>
#include <QWebSocket>

/*@jsdoc
 * Provides a bi-directional, event-driven communication session between the script and another WebSocket connection. It is a 
 * near-complete implementation of the WebSocket API described in the Mozilla docs: 
 * <a href="https://developer.mozilla.org/en-US/docs/Web/API/WebSocket">https://developer.mozilla.org/en-US/docs/Web/API/WebSocket</a>.
 *
 * <p>Create using <code>new WebSocket(...)</code> in Interface, client entity, avatar, and server entity scripts, or the 
 * {@link WebSocketServer} class in server entity and assignment client scripts.
 *
 * <p><strong>Note:</strong> Does not support secure, <code>wss:</code> protocol.</p>
 *
 * @class WebSocket
 * @param {string|WebSocket} urlOrWebSocket - The URL to connect to or an existing {@link WebSocket} to reuse the connection of.
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {string} binaryType="blob" - <em>Not used.</em>
 * @property {number} bufferedAmount=0 - <em>Not implemented.</em> <em>Read-only.</em>
 * @property {string} extensions="" - <em>Not implemented.</em> <em>Read-only.</em>
 *
 * @property {WebSocket~onOpenCallback} onopen - Function called when the connection opens.
 * @property {WebSocket~onMessageCallback} onmessage - Function called when a message is received.
 * @property {WebSocket~onErrorCallback} onerror - Function called when an error occurs.
 * @property {WebSocket~onCloseCallback} onclose - Function called when the connection closes.
 *
 * @property {string} protocol="" - <em>Not implemented.</em> <em>Read-only.</em>
 * @property {WebSocket.ReadyState} readyState - The state of the connection. <em>Read-only.</em>
 * @property {string} url - The URL to connect to. <em>Read-only.</em>
 *
 * @property {WebSocket.ReadyState} CONNECTING - The connection is opening. <em>Read-only.</em>
 * @property {WebSocket.ReadyState} OPEN - The connection is open. <em>Read-only.</em>
 * @property {WebSocket.ReadyState} CLOSING - The connection is closing. <em>Read-only.</em>
 * @property {WebSocket.ReadyState} CLOSED - The connection is closed. <em>Read-only.</em>
 *
 * @example <caption>Echo a message off websocket.org.</caption>
 * print("Create WebSocket");
 * var WEBSOCKET_PING_URL = "ws://echo.websocket.org";
 * var webSocket = new WebSocket(WEBSOCKET_PING_URL);
 * 
 * webSocket.onclose = function (data) {
 *     print("WebSocket closed");
 *     print("Ready state =", webSocket.readyState);  // 3
 * };
 * 
 * webSocket.onmessage = function (data) {
 *     print("Message received:", data.data);
 * 
 *     print("Close WebSocket");
 *     webSocket.close();
 * };
 * 
 * webSocket.onopen = function () {
 *     print("WebSocket opened");
 *     print("Ready state =", webSocket.readyState);  // 1
 * 
 *     print("Send a test message");
 *     webSocket.send("Test message");
 * };
 */
/// Provides the <code><a href="https://apidocs.vircadia.dev/WebSocket.html">WebSocket</a></code> scripting interface
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

    /*@jsdoc
     * The state of a WebSocket connection.
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>CONNECTING</td><td>The connection is opening.</td></tr>
     *     <tr><td><code>1</code></td><td>OPEN</td><td>The connection is open.</td></tr>
     *     <tr><td><code>2</code></td><td>CLOSING</td><td>The connection is closing.</td></tr>
     *     <tr><td><code>3</code></td><td>CLOSED</td><td>The connection is closed.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} WebSocket.ReadyState
     */
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

    /*@jsdoc
     * Sends a message on the connection.
     * @function WebSocket.send
     * @param {string|object} message - The message to send. If an object, it is converted to a string.
     */
    void send(QScriptValue message);

    /*@jsdoc
     * Closes the connection.
     * @function WebSocket.close
     * @param {WebSocket.CloseCode} [closeCode=1000] - The reason for closing.
     * @param {string} [reason=""] - A description of the reason for closing.
     */
    /*@jsdoc
     * The reason why the connection was closed.
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th></tr>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>1000</code></td><td>Normal</td><td>Normal closure.</td></tr>
     *     <tr><td><code>1001</code></td><td>GoingAway</td><td>Going away.</td></tr>
     *     <tr><td><code>1002</code></td><td>ProtocolError</td><td>Protocol error.</td></tr>
     *     <tr><td><code>1003</code></td><td>DatatypeNotSupported</td><td>Unsupported data.</td></tr>
     *     <tr><td><code>1004</code></td><td>Reserved1004</td><td>Reserved.</td></tr>
     *     <tr><td><code>1005</code></td><td>MissingStatusCode</td><td>No status received.</td></tr>
     *     <tr><td><code>1006</code></td><td>AbnormalDisconnection</td><td>abnormal closure.</td></tr>
     *     <tr><td><code>1007</code></td><td>WrongDatatype</td><td>Invalid frame payload data.</td></tr>
     *     <tr><td><code>1008</code></td><td>PolicyViolated</td><td>Policy violation.</td></tr>
     *     <tr><td><code>1009</code></td><td>TooMuchData</td><td>Message too big.</td></tr>
     *     <tr><td><code>1010</code></td><td>MissingExtension</td><td>Mandatory extension missing.</td></tr>
     *     <tr><td><code>1011</code></td><td>BadOperation</td><td>Internal server error.</td></tr>
     *     <tr><td><code>1015</code></td><td>TlsHandshakeFailed</td><td>TLS handshake failed.</td></tr>
     *   </tbody>
     * <table>
     * @typedef {number} WebSocket.CloseCode
     */
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

/// @}
