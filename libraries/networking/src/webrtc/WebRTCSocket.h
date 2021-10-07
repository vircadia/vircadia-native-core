//
//  WebRTCSocket.h
//  libraries/networking/src/webrtc
//
//  Created by David Rowe on 21 Jun 2021.
//  Copyright 2021 Vircadia contributors.
//

#ifndef vircadia_WebRTCSocket_h
#define vircadia_WebRTCSocket_h

#include <shared/WebRTC.h>

#if defined(WEBRTC_DATA_CHANNELS)

#include <QAbstractSocket>
#include <QObject>
#include <QQueue>

#include "WebRTCDataChannels.h"

/// @addtogroup Networking
/// @{


/// @brief Provides a QUdpSocket-style interface for using WebRTCDataChannels.
///
/// @details A WebRTC data channel is identified by the IP address and port of the client WebSocket that was used when opening
/// the data channel - this is considered to be the WebRTC data channel's address. The IP address and port of the actual WebRTC
/// connection is not used.
class WebRTCSocket : public QObject {
    Q_OBJECT

public:

    /// @brief Constructs a new WebRTCSocket object.
    /// @param parent Qt parent object.
    WebRTCSocket(QObject* parent);


    /// @brief Nominally sets the value of a socket option.
    /// @details Only <code>SendBufferSizeSocketOption</code> and <code>ReceiveBufferSizeSocketOption</code> options are handled
    /// and for these no action is taken because these buffer sizes are not configurable in WebRTC.
    /// Included for compatibility with the QUdpSocket interface.
    /// @param option The socket option.
    /// @param value The value of the socket option.
    void setSocketOption(QAbstractSocket::SocketOption option, const QVariant& value);

    /// @brief Nominally gets the value of a socket option.
    /// @details Only <code>SendBufferSizeSocketOption</code> and <code>ReceiveBufferSizeSocketOption</code> options are handled
    /// and for these only default values are returned because these buffer sizes are not configurable in WebRTC.
    /// Included for compatibility with the QUdpSocket interface.
    /// @param option The socket option.
    /// @return The value of the socket option.
    QVariant socketOption(QAbstractSocket::SocketOption option);

    /// @brief Nominally binds the WebRTC socket to an address and port.
    /// @details WebRTC data connections aren't actually bound to an address or port. Their ports are negotiated as part of the
    /// WebRTC peer connection process.
    /// Included for compatibility with the QUdpSocket interface.
    /// @param address The address.
    /// @param port The port.
    /// @param mode The bind mode.
    /// @return <code>true</code>.
    bool bind(const QHostAddress& address, quint16 port = 0, QAbstractSocket::BindMode mode
        = QAbstractSocket::DefaultForPlatform);

    /// @brief Gets the state of the socket.
    /// @details In particular, QAbstractSocket::BoundState is returned if the socket is bound, 
    /// QAbstractSocket::UnconnectedState if it isn't.
    /// @return The state of the socket.
    QAbstractSocket::SocketState state() const;

    /// @brief Immediately closes all connections and resets the socket.
    void abort();

    /// @brief Nominally gets the host port number.
    /// Included for compatibility with the QUdpSocket interface.
    /// @return <code>0</code> 
    quint16 localPort() const { return 0; }

    /// @brief Nominally gets the socket descriptor.
    /// Included for compatibility with the QUdpSocket interface.
    /// @return <code>-1</code>
    qintptr socketDescriptor() const { return -1; }


    /// @brief Sends a datagram.
    /// @param datagram The datagram to send.
    /// @param destination The destination WebRTC data channel address.
    /// @return The number of bytes if successfully sent, otherwise <code>-1</code>.
    qint64 writeDatagram(const QByteArray& datagram, const SockAddr& destination);

    /// @brief Gets the number of bytes waiting to be written.
    /// @param destination The destination WebRTC data channel address.
    /// @return The number of bytes waiting to be written.
    qint64 bytesToWrite(const SockAddr& destination) const;

    /// @brief Gets whether there's a datagram waiting to be read.
    /// @return <code>true</code> if there's a datagram waiting to be read, <code>false</code> if there isn't.
    bool hasPendingDatagrams() const;

    /// @brief Gets the size of the first pending datagram.
    /// @return the size of the first pending datagram; <code>-1</code> if there is no pending datagram.
    qint64 pendingDatagramSize() const;

    /// @brief Reads the next datagram, up to a maximum number of bytes.
    /// @details Any remaining data in the datagram is lost.
    /// @param data The destination to read the datagram into.
    /// @param maxSize The maximum number of bytes to read.
    /// @param address The destination to put the WebRTC data channel's IP address.
    /// @param port The destination to put the WebRTC data channel's port.
    /// @return The number of bytes read on success; <code>-1</code> if reading unsuccessful.
    qint64 readDatagram(char* data, qint64 maxSize, QHostAddress* address = nullptr, quint16* port = nullptr);


    /// @brief Gets the type of error that last occurred.
    /// @return The type of error that last occurred.
    QAbstractSocket::SocketError error() const;

    /// @brief Gets the description of the error that last occurred.
    /// @return The description of the error that last occurred.
    QString errorString() const;

public slots:

    /// @brief Handles the WebRTC data channel receiving a message.
    /// @details Queues the message to be read via readDatagram.
    /// @param source The WebRTC data channel that the message was received on.
    /// @param message The message that was received.
    void onDataChannelReceivedMessage(const SockAddr& source, const QByteArray& message);

signals:

    /// @brief Emitted when the state of the socket changes.
    /// @param socketState The new state of the socket.
    void stateChanged(QAbstractSocket::SocketState socketState);

    /// @brief Emitted each time new data becomes available for reading.
    void readyRead();

    /// @brief Emitted when a WebRTC signaling message has been received from the signaling server for this WebRTCSocket.
    /// @param json The signaling message.
    void onSignalingMessage(const QJsonObject& json);

    /// @brief Emitted when there's a WebRTC signaling message to send via the signaling server.
    /// @param json The signaling message.
    void sendSignalingMessage(const QJsonObject& message);


private:

    void setError(QAbstractSocket::SocketError errorType, QString errorString);
    void clearError();

    WebRTCDataChannels _dataChannels;

    bool _isBound { false };

    QQueue<QPair<SockAddr, QByteArray>> _receivedQueue;  // Messages received are queued for reading from the "socket".

    QAbstractSocket::SocketError _lastErrorType { QAbstractSocket::UnknownSocketError };
    QString _lastErrorString;
};


/// @}

#endif // WEBRTC_DATA_CHANNELS

#endif // vircadia_WebRTCSocket_h
