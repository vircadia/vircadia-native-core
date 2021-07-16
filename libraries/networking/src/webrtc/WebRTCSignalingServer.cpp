//
//  WebRTCSignalingServer.cpp
//  libraries/networking/src/webrtc
//
//  Created by David Rowe on 16 May 2021.
//  Copyright 2021 Vircadia contributors.
//

#include "WebRTCSignalingServer.h"

#if defined(WEBRTC_DATA_CHANNELS)

#include <QtCore>
#include <QWebSocket>

#include "../NetworkLogging.h"
#include "../NodeType.h"


const int WEBRTC_SOCKET_CHECK_INTERVAL_IN_MS = 30000;

WebRTCSignalingServer::WebRTCSignalingServer(QObject* parent) :
    QObject(parent),
    _webSocketServer(new QWebSocketServer(QStringLiteral("WebRTC Signaling Server"), QWebSocketServer::NonSecureMode, this))
{
    connect(_webSocketServer, &QWebSocketServer::newConnection, this, &WebRTCSignalingServer::newWebSocketConnection);

    // Automatically recover from network interruptions.
    _isWebSocketServerListeningTimer = new QTimer(this);
    connect(_isWebSocketServerListeningTimer, &QTimer::timeout, this, &WebRTCSignalingServer::checkWebSocketServerIsListening);
    _isWebSocketServerListeningTimer->start(WEBRTC_SOCKET_CHECK_INTERVAL_IN_MS);
}

bool WebRTCSignalingServer::bind(const QHostAddress& address, quint16 port) {
    _address = address;
    _port = port;
    auto success = _webSocketServer->listen(_address, _port);
    if (!success) {
        qCWarning(networking_webrtc) << "Failed to open WebSocket for WebRTC signaling.";
    }
    return success;
}

void WebRTCSignalingServer::checkWebSocketServerIsListening() {
    if (!_webSocketServer->isListening()) {
        qCWarning(networking_webrtc) << "WebSocket on port " << QString::number(_port) << " is no longer listening";
        _webSockets.clear();
        _webSocketServer->listen(_address, _port);
    }
}

void WebRTCSignalingServer::webSocketTextMessageReceived(const QString& message) {
    auto source = qobject_cast<QWebSocket*>(sender());
    if (source) {
        QJsonObject json = QJsonDocument::fromJson(message.toUtf8()).object();
        // WEBRTC TODO: Move domain server echoing into domain server.
        if (json.keys().contains("echo") && json.value("to").toString() == QString(QChar(NodeType::DomainServer))) {
            // Domain server echo request - echo message back to sender.
            json.remove("to");
            json.insert("from", QString(QChar(NodeType::DomainServer)));
            QString echo = QJsonDocument(json).toJson();
            source->sendTextMessage(echo);
        } else {
            // WebRTC message or assignment client echo request. (Send both to target.)
            json.insert("from", source->peerPort());
            emit messageReceived(json);
        }
    } else {
        qCWarning(networking_webrtc) << "Failed to find WebSocket for incoming WebRTC signaling message.";
    }
}

void WebRTCSignalingServer::sendMessage(const QJsonObject& message) {
    quint16 destinationPort = message.value("to").toInt();
    if (_webSockets.contains(destinationPort)) {
        _webSockets.value(destinationPort)->sendTextMessage(QString(QJsonDocument(message).toJson()));
    } else {
        qCWarning(networking_webrtc) << "Failed to find WebSocket for outgoing WebRTC signaling message.";
    }
}

void WebRTCSignalingServer::webSocketDisconnected() {
    auto source = qobject_cast<QWebSocket*>(sender());
    if (source) {
        _webSockets.remove(source->peerPort());
    }
}

void WebRTCSignalingServer::newWebSocketConnection() {
    auto webSocket = _webSocketServer->nextPendingConnection();
    connect(webSocket, &QWebSocket::textMessageReceived, this, &WebRTCSignalingServer::webSocketTextMessageReceived);
    connect(webSocket, &QWebSocket::disconnected, this, &WebRTCSignalingServer::webSocketDisconnected);
    _webSockets.insert(webSocket->peerPort(), webSocket);
}

#endif // WEBRTC_DATA_CHANNELS
