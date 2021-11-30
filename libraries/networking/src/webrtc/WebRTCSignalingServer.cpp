//
//  WebRTCSignalingServer.cpp
//  libraries/networking/src/webrtc
//
//  Created by David Rowe on 16 May 2021.
//  Copyright 2021 Vircadia contributors.
//

#include "WebRTCSignalingServer.h"

#if defined(WEBRTC_DATA_CHANNELS)

#include <QSslKey>
#include <QtCore>
#include <QWebSocket>

#include <BuildInfo.h>
#include <PathUtils.h>

#include "../NetworkLogging.h"
#include "../NodeType.h"


const int WEBRTC_SOCKET_CHECK_INTERVAL_IN_MS = 30000;

WebRTCSignalingServer::WebRTCSignalingServer(QObject* parent, bool isWSSEnabled) :
    QObject(parent)
{
    if (isWSSEnabled) {
        _webSocketServer = (new QWebSocketServer(QStringLiteral("WebRTC Signaling Server"), QWebSocketServer::SecureMode,
            this));

        auto dsDirPath = PathUtils::getAppLocalDataPath();
        const QString KEY_FILENAME = "vircadia-cert.key";
        const QString CRT_FILENAME = "vircadia-cert.crt";
        const QString CA_CRT_FILENAME = "vircadia-cert-ca.crt";
        qCDebug(networking_webrtc) << "WebSocket WSS key file:" << dsDirPath + KEY_FILENAME;
        qCDebug(networking_webrtc) << "WebSocket WSS cert file:" << dsDirPath + CRT_FILENAME;
        qCDebug(networking_webrtc) << "WebSocket WSS CA cert file:" << dsDirPath + CA_CRT_FILENAME;

        QFile sslCaFile(dsDirPath + CA_CRT_FILENAME);
        sslCaFile.open(QIODevice::ReadOnly);
        QSslCertificate sslCaCertificate(&sslCaFile, QSsl::Pem);
        sslCaFile.close();

        QSslConfiguration sslConfiguration;
        QFile sslCrtFile(dsDirPath + CRT_FILENAME);
        sslCrtFile.open(QIODevice::ReadOnly);
        QSslCertificate sslCertificate(&sslCrtFile, QSsl::Pem);
        sslCrtFile.close();

        QFile sslKeyFile(dsDirPath + KEY_FILENAME);
        sslKeyFile.open(QIODevice::ReadOnly);
        QSslKey sslKey(&sslKeyFile, QSsl::Rsa, QSsl::Pem);
        sslKeyFile.close();

        if (!sslCaCertificate.isNull() && !sslKey.isNull() && !sslCertificate.isNull()) {
            sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
            sslConfiguration.addCaCertificate(sslCaCertificate);
            sslConfiguration.setLocalCertificate(sslCertificate);
            sslConfiguration.setPrivateKey(sslKey);
            _webSocketServer->setSslConfiguration(sslConfiguration);
            qCDebug(networking_webrtc) << "WebSocket SSL mode enabled:"
                << (_webSocketServer->secureMode() == QWebSocketServer::SecureMode);
        } else {
            qCWarning(networking_webrtc) << "Error creating WebSocket SSL key.";
        }

    } else {
        _webSocketServer = (new QWebSocketServer(QStringLiteral("WebRTC Signaling Server"), QWebSocketServer::NonSecureMode,
            this));
    }
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
            auto from = source->peerAddress().toString() + ":" + QString::number(source->peerPort());
            json.insert("from", from);
            emit messageReceived(json);
        }
    } else {
        qCWarning(networking_webrtc) << "Failed to find WebSocket for incoming WebRTC signaling message.";
    }
}

void WebRTCSignalingServer::sendMessage(const QJsonObject& message) {
    auto destinationAddress = message.value("to").toString();
    if (_webSockets.contains(destinationAddress)) {
        _webSockets.value(destinationAddress)->sendTextMessage(QString(QJsonDocument(message).toJson()));
    } else {
        qCWarning(networking_webrtc) << "Failed to find WebSocket for outgoing WebRTC signaling message.";
    }
}

void WebRTCSignalingServer::webSocketDisconnected() {
    auto source = qobject_cast<QWebSocket*>(sender());
    if (source) {
        auto address = source->peerAddress().toString() + ":" + QString::number(source->peerPort());
        _webSockets.remove(address);
        source->deleteLater();
    }
}

void WebRTCSignalingServer::newWebSocketConnection() {
    auto webSocket = _webSocketServer->nextPendingConnection();
    connect(webSocket, &QWebSocket::textMessageReceived, this, &WebRTCSignalingServer::webSocketTextMessageReceived);
    connect(webSocket, &QWebSocket::disconnected, this, &WebRTCSignalingServer::webSocketDisconnected);
    auto webSocketAddress = webSocket->peerAddress().toString() + ":" + QString::number(webSocket->peerPort());
    _webSockets.insert(webSocketAddress, webSocket);
}

#endif // WEBRTC_DATA_CHANNELS
