//
//  WebRTCSignalingServer.h
//  libraries/networking/src/webrtc
//
//  Created by David Rowe on 16 May 2021.
//  Copyright 2021 Vircadia contributors.
//

#ifndef vircadia_WebRTCSignalingServer_h
#define vircadia_WebRTCSignalingServer_h

#include <shared/WebRTC.h>

#if defined(WEBRTC_DATA_CHANNELS)

#include <QObject>
#include <QtCore/QTimer>
#include <QWebSocketServer>

#include "../SockAddr.h"

/// @addtogroup Networking
/// @{

/// @brief Provides a WebRTC signaling server that Interface clients can use to initiate WebRTC connections to the domain server
/// and its assignment clients.
/// 
/// @details The signaling server is expected to be hosted in the domain server. It provides a WebSocket for Interface clients
/// to use in the WebRTC signaling handshake process to establish WebRTC data channel connections to each of the domain server
/// and the assignment clients (i.e., separate WebRTC data channels for each but only a single signaling WebSocket). The
/// assignment client signaling messages are expected to be relayed - by the domain server - via Vircadia protocol messages on
/// the UDP connections between the domain server and assignment clients.
/// 
/// Additionally, for debugging purposes, instead of containing a WebRTC payload a signaling message may be an echo request.
/// This is bounced back to the client from the WebRTCSignalingServer if the domain server was the target, otherwise it is
/// expected to be bounced back upon receipt by the relevant assignment client.
///
/// The signaling messages are sent and received as JSON objects, with `to` and `from` fields in addition to either the WebRTC
/// signaling `data` payload or an `echo` request:
///
/// | Interface -> Server ||
/// | -------- | ---------------------------------------- |
/// | `to`     | NodeType                                 |
/// | `from`   | WebSocket IP address & port, "n.n.n.n:n" |
/// | [`data`] | WebRTC signaling payload                 |
/// | [`echo`] | Echo request                             |
///
/// `*` The `from` field is filled in upon receipt by the WebRTCSignalingServer.
///
/// | Server -> Interface ||
/// | -------- | ---------------------------------------- |
/// | `to`     | WebSocket IP address & port, "n.n.n.n:n" |
/// | `from`   | NodeType                                 |
/// | [`data`] | WebRTC signaling payload                 |
/// | [`echo`] | Echo response                            |
///
class WebRTCSignalingServer : public QObject {
    Q_OBJECT

public:

    /// @brief Constructs a new WebRTCSignalingServer object.
    /// @param parent Qt parent object.
    /// @param isWSSEnabled Whether the WebSocket used for WebRTC signaling should be secure (WSS protocol).
    WebRTCSignalingServer(QObject* parent, bool isWSSEnabled);

    /// @brief Binds the WebRTC signaling server's WebSocket to an address and port.
    /// @param address The address to use for the WebSocket.
    /// @param port The port to use for the WebSocket.
    /// @return <code>true</code> if the WebSocket was successfully bound, <code>false</code> if it wasn't.
    bool bind(const QHostAddress& address, quint16 port);

public slots:

    /// @brief Send a WebRTC signaling channel message to an Interface client.
    /// @param message The message to send to the Interface client. Includes details of the sender and the destination in
    /// addition to the WebRTC signaling channel payload.
    void sendMessage(const QJsonObject& message);

signals:

    /// @brief A WebRTC signaling channel message was received from an Interface client.
    /// @param message The message received from the Interface client. Includes details of the sender and the destination in
    /// addition to the WebRTC signaling channel payload.\n
    /// Not emitted if the message was an echo request for the domain server.
    void messageReceived(const QJsonObject& message);

private slots:

    void newWebSocketConnection();
    void webSocketTextMessageReceived(const QString& message);
    void webSocketDisconnected();

private:

    void checkWebSocketServerIsListening();

    QWebSocketServer* _webSocketServer;
    QHostAddress _address;
    quint16 _port { 0 };

    QHash<QString, QWebSocket*> _webSockets;  // <client WebSocket IP address and port, client connection WebSocket object>
    // The WebSocket IP address and port is formatted as "n.n.n.n:n".
    // A QString is used rather than a SockAddr, to make signaling easier.

    QTimer* _isWebSocketServerListeningTimer;
};

/// @}

#endif // WEBRTC_DATA_CHANNELS

#endif // vircadia_WebRTCSignalingServer_h
