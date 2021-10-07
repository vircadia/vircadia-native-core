//
//  NetworkSocket.h
//  libraries/networking/src/udt
//
//  Created by David Rowe on 21 Jun 2021.
//  Copyright 2021 Vircadia contributors.
//

#ifndef vircadia_NetworkSocket_h
#define vircadia_NetworkSocket_h

#include <QObject>
#include <QUdpSocket>

#include <shared/WebRTC.h>

#include "../SockAddr.h"
#include "../NodeType.h"
#include "../SocketType.h"
#if defined(WEBRTC_DATA_CHANNELS)
#include "../webrtc/WebRTCSocket.h"
#endif

/// @addtogroup Networking
/// @{


/// @brief Multiplexes a QUdpSocket and a WebRTCSocket so that they appear as a single QUdpSocket-style socket.
class NetworkSocket : public QObject {
    Q_OBJECT

public:

    /// @brief Constructs a new NetworkSocket object.
    /// @param parent Qt parent object.
    NetworkSocket(QObject* parent);


    /// @brief Set the value of a UDP or WebRTC socket option.
    /// @param socketType The type of socket for which to set the option value.
    /// @param option The option to set the value of.
    /// @param value The option value.
    void setSocketOption(SocketType socketType, QAbstractSocket::SocketOption option, const QVariant& value);
    
    /// @brief Gets the value of a UDP or WebRTC socket option.
    /// @param socketType The type of socket for which to get the option value.
    /// @param option The option to get the value of.
    /// @return The option value.
    QVariant socketOption(SocketType socketType, QAbstractSocket::SocketOption option);


    /// @brief Binds the UDP or WebRTC socket to an address and port.
    /// @param socketType The type of socket to bind.
    /// @param address The address to bind to.
    /// @param port The port to bind to.
    void bind(SocketType socketType, const QHostAddress& address, quint16 port = 0);
    
    /// @brief Immediately closes and resets the socket.
    /// @param socketType The type of socket to close and reset.
    void abort(SocketType socketType);


    /// @brief Gets the UDP or WebRTC local port number.
    /// @param socketType The type of socket for which to the get local port number.
    /// @return The UDP or WebRTC local port number if available, otherwise <code>0</code>.
    quint16 localPort(SocketType socketType) const;

    /// @brief Returns the native socket descriptor of the UDP or WebRTC socket.
    /// @param socketType The type of socket to get the socket descriptor for.
    /// @return The native socket descriptor if available, otherwise <code>-1</code>.
    qintptr socketDescriptor(SocketType socketType) const;


    /// @brief Sends a datagram to a network address.
    /// @param datagram The datagram to send.
    /// @param sockAddr The address to send to.
    /// @return The number of bytes if successfully sent, otherwise <code>-1</code>.
    qint64 writeDatagram(const QByteArray& datagram, const SockAddr& sockAddr);

    /// @brief Gets the number of bytes waiting to be written.
    /// @details For UDP, there's a single buffer used for all destinations. For WebRTC, each destination has its own buffer.
    /// @param socketType The type of socket for which to get the number of bytes waiting to be written.
    /// @param address If a WebRTCSocket, the destination address for which to get the number of bytes waiting.
    /// @param port If a WebRTC socket, the destination port for which to get the number of bytes waiting.
    /// @return The number of bytes waiting to be written.
    qint64 bytesToWrite(SocketType socketType, const SockAddr& address = SockAddr()) const;


    /// @brief Gets whether there is a pending datagram waiting to be read.
    /// @return <code>true</code> if there is a datagram waiting to be read, <code>false</code> if there isn't.
    bool hasPendingDatagrams() const;
    
    /// @brief Gets the size of the next pending datagram, alternating between socket types if both have datagrams to read.
    /// @return The size of the next pending datagram.
    qint64 pendingDatagramSize();
    
    /// @brief Reads the next datagram per the most recent pendingDatagramSize call if made, otherwise alternating between
    /// socket types if both have datagrams to read.
    /// @param data The destination to write the data into.
    /// @param maxSize The maximum number of bytes to read.
    /// @param sockAddr The destination to write the source network address into.
    /// @return The number of bytes if successfully read, otherwise <code>-1</code>.
    qint64 readDatagram(char* data, qint64 maxSize, SockAddr* sockAddr = nullptr);

    
    /// @brief Gets the state of the UDP or WebRTC socket.
    /// @param socketType The type of socket for which to get the state.
    /// @return The socket state.
    QAbstractSocket::SocketState state(SocketType socketType) const;


    /// @brief Gets the type of error that last occurred.
    /// @param socketType The type of socket for which to get the last error.
    /// @return The type of error that last occurred.
    QAbstractSocket::SocketError error(SocketType socketType) const;

    /// @brief Gets the description of the error that last occurred.
    /// @param socketType The type of socket for which to get the last error's description.
    /// @return The description of the error that last occurred.
    QString errorString(SocketType socketType) const;


#if defined(WEBRTC_DATA_CHANNELS)
    /// @brief Gets a pointer to the WebRTC socket object.
    /// @return A pointer to the WebRTC socket object.
    const WebRTCSocket* getWebRTCSocket();
#endif

signals:

    /// @brief Emitted each time new data becomes available for reading.
    void readyRead();

    /// @brief Emitted when the state of the underlying UDP or WebRTC socket changes.
    /// @param socketType The type of socket that changed state.
    /// @param socketState The socket's new state.
    void stateChanged(SocketType socketType, QAbstractSocket::SocketState socketState);

    /// @brief 
    /// @param socketType 
    /// @param socketError 
    void socketError(SocketType socketType, QAbstractSocket::SocketError socketError);

private slots:

    void onUDPStateChanged(QAbstractSocket::SocketState socketState);
    void onWebRTCStateChanged(QAbstractSocket::SocketState socketState);

    void onUDPSocketError(QAbstractSocket::SocketError socketError);
    void onWebRTCSocketError(QAbstractSocket::SocketError socketError);

private:

    QObject* _parent;

    QUdpSocket _udpSocket;
#if defined(WEBRTC_DATA_CHANNELS)
    WebRTCSocket _webrtcSocket;
#endif

#if defined(WEBRTC_DATA_CHANNELS)
    SocketType _pendingDatagramSizeSocketType { SocketType::Unknown };
    SocketType _lastSocketTypeRead { SocketType::Unknown };
#endif
};


/// @}

#endif // vircadia_NetworkSocket_h
