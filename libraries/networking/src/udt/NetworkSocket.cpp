//
//  NetworkSocket.cpp
//  libraries/networking/src/udt
//
//  Created by David Rowe on 21 Jun 2021.
//  Copyright 2021 Vircadia contributors.
//

#include "NetworkSocket.h"

#include "../NetworkLogging.h"


NetworkSocket::NetworkSocket(QObject* parent) :
    QObject(parent),
    _parent(parent),
    _udpSocket(this)
#if defined(WEBRTC_DATA_CHANNELS)
    ,
    _webrtcSocket(this)
#endif
{
    connect(&_udpSocket, &QUdpSocket::readyRead, this, &NetworkSocket::readyRead);
    connect(&_udpSocket, &QAbstractSocket::stateChanged, this, &NetworkSocket::onUDPStateChanged);
    // Use old SIGNAL/SLOT mechanism for Android builds.
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
        this, SLOT(onUDPSocketError(QAbstractSocket::SocketError)));

#if defined(WEBRTC_DATA_CHANNELS)
    connect(&_webrtcSocket, &WebRTCSocket::readyRead, this, &NetworkSocket::readyRead);
    connect(&_webrtcSocket, &WebRTCSocket::stateChanged, this, &NetworkSocket::onWebRTCStateChanged);
    // WEBRTC TODO: Add similar for errorOccurred
#endif
}


void NetworkSocket::setSocketOption(SocketType socketType, QAbstractSocket::SocketOption option, const QVariant& value) {
    switch (socketType) {
    case SocketType::UDP:
        _udpSocket.setSocketOption(option, value);
        break;
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        _webrtcSocket.setSocketOption(option, value);
        break;
#endif
    default:
        qCCritical(networking) << "Socket type not specified in setSocketOption()";
    }
}

QVariant NetworkSocket::socketOption(SocketType socketType, QAbstractSocket::SocketOption option) {
    switch (socketType) {
    case SocketType::UDP:
        return _udpSocket.socketOption(option);
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.socketOption(option);
#endif
    default:
        qCCritical(networking) << "Socket type not specified in socketOption()";
        return "";
    }
}


void NetworkSocket::bind(SocketType socketType, const QHostAddress& address, quint16 port) {
    switch (socketType) {
    case SocketType::UDP:
        _udpSocket.bind(address, port);
        break;
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        _webrtcSocket.bind(address, port);
        break;
#endif
    default:
        qCCritical(networking) << "Socket type not specified in bind()";
    }
}

void NetworkSocket::abort(SocketType socketType) {
    switch (socketType) {
    case SocketType::UDP:
        _udpSocket.abort();
        break;
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        _webrtcSocket.abort();
        break;
#endif
    default:
        qCCritical(networking) << "Socket type not specified in abort()";
    }
}


quint16 NetworkSocket::localPort(SocketType socketType) const {
    switch (socketType) {
    case SocketType::UDP:
        return _udpSocket.localPort();
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.localPort();
#endif
    default:
        qCCritical(networking) << "Socket type not specified in localPort()";
        return 0;
    }
}

qintptr NetworkSocket::socketDescriptor(SocketType socketType) const {
    switch (socketType) {
    case SocketType::UDP:
        return _udpSocket.socketDescriptor();
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.socketDescriptor();
        return 0;
#endif
    default:
        qCCritical(networking) << "Socket type not specified in socketDescriptor()";
        return 0;
    }
}


qint64 NetworkSocket::writeDatagram(const QByteArray& datagram, const SockAddr& sockAddr) {
    switch (sockAddr.getType()) {
    case SocketType::UDP:
        // WEBRTC TODO: The Qt documentation says that the following call shouldn't be used if the UDP socket is connected!!!
        // https://doc.qt.io/qt-5/qudpsocket.html#writeDatagram
        return _udpSocket.writeDatagram(datagram, sockAddr.getAddress(), sockAddr.getPort());
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.writeDatagram(datagram, sockAddr);
#endif
    default:
        qCCritical(networking) << "Socket type not specified in writeDatagram() address";
        return 0;
    }
}

qint64 NetworkSocket::bytesToWrite(SocketType socketType, const SockAddr& address) const {
    switch (socketType) {
    case SocketType::UDP:
        return _udpSocket.bytesToWrite();
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.bytesToWrite(address);
#endif
    default:
        qCCritical(networking) << "Socket type not specified in bytesToWrite()";
        return 0;
    }
}


bool NetworkSocket::hasPendingDatagrams() const {
    return 
#if defined(WEBRTC_DATA_CHANNELS)
        _webrtcSocket.hasPendingDatagrams() ||
#endif
        _udpSocket.hasPendingDatagrams();
}

qint64 NetworkSocket::pendingDatagramSize() {
#if defined(WEBRTC_DATA_CHANNELS)
    // Alternate socket types, remembering the socket type used so that the same socket type is used next readDatagram().
    if (_lastSocketTypeRead == SocketType::UDP) {
        if (_webrtcSocket.hasPendingDatagrams()) {
            _pendingDatagramSizeSocketType = SocketType::WebRTC;
            return _webrtcSocket.pendingDatagramSize();
        } else {
            _pendingDatagramSizeSocketType = SocketType::UDP;
            return _udpSocket.pendingDatagramSize();
        }
    } else {
        if (_udpSocket.hasPendingDatagrams()) {
            _pendingDatagramSizeSocketType = SocketType::UDP;
            return _udpSocket.pendingDatagramSize();
        } else {
            _pendingDatagramSizeSocketType = SocketType::WebRTC;
            return _webrtcSocket.pendingDatagramSize();
        }
    }
#else
    return _udpSocket.pendingDatagramSize();
#endif
}

qint64 NetworkSocket::readDatagram(char* data, qint64 maxSize, SockAddr* sockAddr) {
#if defined(WEBRTC_DATA_CHANNELS)
    // Read per preceding pendingDatagramSize() if any, otherwise alternate socket types.
    if (_pendingDatagramSizeSocketType == SocketType::UDP
        || _pendingDatagramSizeSocketType == SocketType::Unknown && _lastSocketTypeRead == SocketType::WebRTC) {
        _lastSocketTypeRead = SocketType::UDP;
        _pendingDatagramSizeSocketType = SocketType::Unknown;
        if (sockAddr) {
            sockAddr->setType(SocketType::UDP);
            return _udpSocket.readDatagram(data, maxSize, sockAddr->getAddressPointer(), sockAddr->getPortPointer());
        } else {
            return _udpSocket.readDatagram(data, maxSize);
        }
    } else {
        _lastSocketTypeRead = SocketType::WebRTC;
        _pendingDatagramSizeSocketType = SocketType::Unknown;
        if (sockAddr) {
            sockAddr->setType(SocketType::WebRTC);
            return _webrtcSocket.readDatagram(data, maxSize, sockAddr->getAddressPointer(), sockAddr->getPortPointer());
        } else {
            return _webrtcSocket.readDatagram(data, maxSize);
        }
    }
#else
    if (sockAddr) {
        sockAddr->setType(SocketType::UDP);
        return _udpSocket.readDatagram(data, maxSize, sockAddr->getAddressPointer(), sockAddr->getPortPointer());
    } else {
        return _udpSocket.readDatagram(data, maxSize);
    }
#endif
}


QAbstractSocket::SocketState NetworkSocket::state(SocketType socketType) const {
    switch (socketType) {
    case SocketType::UDP:
        return _udpSocket.state();
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.state();
#endif
    default:
        qCCritical(networking) << "Socket type not specified in state()";
        return QAbstractSocket::SocketState::UnconnectedState;
    }
}


QAbstractSocket::SocketError NetworkSocket::error(SocketType socketType) const {
    switch (socketType) {
    case SocketType::UDP:
        return _udpSocket.error();
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.error();
#endif
    default:
        qCCritical(networking) << "Socket type not specified in error()";
        return QAbstractSocket::SocketError::UnknownSocketError;
    }
}

QString NetworkSocket::errorString(SocketType socketType) const {
    switch (socketType) {
    case SocketType::UDP:
        return _udpSocket.errorString();
#if defined(WEBRTC_DATA_CHANNELS)
    case SocketType::WebRTC:
        return _webrtcSocket.errorString();
#endif
    default:
        qCCritical(networking) << "Socket type not specified in errorString()";
        return "";
    }
}


#if defined(WEBRTC_DATA_CHANNELS)
const WebRTCSocket* NetworkSocket::getWebRTCSocket() {
    return &_webrtcSocket;
}
#endif


void NetworkSocket::onUDPStateChanged(QAbstractSocket::SocketState socketState) {
    emit stateChanged(SocketType::UDP, socketState);
}

void NetworkSocket::onWebRTCStateChanged(QAbstractSocket::SocketState socketState) {
    emit stateChanged(SocketType::WebRTC, socketState);
}

void NetworkSocket::onUDPSocketError(QAbstractSocket::SocketError socketError) {
    emit NetworkSocket::socketError(SocketType::UDP, socketError);
}

void NetworkSocket::onWebRTCSocketError(QAbstractSocket::SocketError socketError) {
    emit NetworkSocket::socketError(SocketType::WebRTC, socketError);
}
