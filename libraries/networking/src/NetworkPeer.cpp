//
//  NetworkPeer.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-10-02.
//  Copyright 2014 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NetworkPeer.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDataStream>

#include <SharedUtil.h>
#include <UUID.h>

#include "NetworkLogging.h"
#include <Trace.h>
#include "NodeType.h"

const NetworkPeer::LocalID NetworkPeer::NULL_LOCAL_ID;

NetworkPeer::NetworkPeer(QObject* parent) :
    QObject(parent),
    _uuid(),
    _publicSocket(),
    _localSocket(),
    _symmetricSocket(),
    _activeSocket(NULL),
    _wakeTimestamp(QDateTime::currentMSecsSinceEpoch()),
    _connectionAttempts(0)
{
    _lastHeardMicrostamp = usecTimestampNow();
}

NetworkPeer::NetworkPeer(const QUuid& uuid, const SockAddr& publicSocket, const SockAddr& localSocket, QObject* parent) :
    QObject(parent),
    _uuid(uuid),
    _publicSocket(publicSocket),
    _localSocket(localSocket),
    _symmetricSocket(),
    _activeSocket(NULL),
    _wakeTimestamp(QDateTime::currentMSecsSinceEpoch()),
    _connectionAttempts(0)
{
    _lastHeardMicrostamp = usecTimestampNow();
}

void NetworkPeer::setPublicSocket(const SockAddr& publicSocket) {
    if (publicSocket != _publicSocket) {
        if (_activeSocket == &_publicSocket) {
            // if the active socket was the public socket then reset it to NULL
            _activeSocket = NULL;
        }
        
        bool wasOldSocketNull = _publicSocket.isNull();

        auto previousSocket = _publicSocket;
        _publicSocket = publicSocket;
        _publicSocket.setObjectName(previousSocket.objectName());
        
        if (!wasOldSocketNull) {
            qCDebug(networking) << "Public socket change for node" << *this << "; previously" << previousSocket;
            emit socketUpdated(previousSocket, _publicSocket);
        }
    }
}

void NetworkPeer::setLocalSocket(const SockAddr& localSocket) {
    if (localSocket != _localSocket) {
        if (_activeSocket == &_localSocket) {
            // if the active socket was the local socket then reset it to NULL
            _activeSocket = NULL;
        }
        
        bool wasOldSocketNull = _localSocket.isNull();
        
        auto previousSocket = _localSocket;
        _localSocket = localSocket;
        _localSocket.setObjectName(previousSocket.objectName());

        if (!wasOldSocketNull) {
            qCDebug(networking) << "Local socket change for node" << *this << "; previously" << previousSocket;
            emit socketUpdated(previousSocket, _localSocket);
        }
    }
}

void NetworkPeer::setSymmetricSocket(const SockAddr& symmetricSocket) {
    if (symmetricSocket != _symmetricSocket) {
        if (_activeSocket == &_symmetricSocket) {
            // if the active socket was the symmetric socket then reset it to NULL
            _activeSocket = NULL;
        }
        
        bool wasOldSocketNull = _symmetricSocket.isNull();
        
        auto previousSocket = _symmetricSocket;
        _symmetricSocket = symmetricSocket;
        _symmetricSocket.setObjectName(previousSocket.objectName());
        
        if (!wasOldSocketNull) {
            qCDebug(networking) << "Symmetric socket change for node" << *this << "; previously" << previousSocket;
            emit socketUpdated(previousSocket, _symmetricSocket);
        }
    }
}

void NetworkPeer::setActiveSocket(SockAddr* discoveredSocket) {
    _activeSocket = discoveredSocket;

    // we have an active socket, stop our ping timer
    stopPingTimer();

    // we're now considered connected to this peer - reset the number of connection attempts
    resetConnectionAttempts();
    
    if (_activeSocket) {
        emit socketActivated(*_activeSocket);
    }
}

void NetworkPeer::activateLocalSocket() {
    if (_activeSocket != &_localSocket) {
        qCDebug(networking) << "Activating local socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
        setActiveSocket(&_localSocket);
    }
}

void NetworkPeer::activatePublicSocket() {
    if (_activeSocket != &_publicSocket) {
        qCDebug(networking) << "Activating public socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
        setActiveSocket(&_publicSocket);
    }
}

void NetworkPeer::activateSymmetricSocket() {
    if (_activeSocket != &_symmetricSocket) {
        qCDebug(networking) << "Activating symmetric socket (" << _symmetricSocket << ") for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
        setActiveSocket(&_symmetricSocket);
    }
}

void NetworkPeer::activateMatchingOrNewSymmetricSocket(const SockAddr& matchableSockAddr) {
    if (matchableSockAddr == _publicSocket) {
        activatePublicSocket();
    } else if (matchableSockAddr == _localSocket) {
        activateLocalSocket();
    } else {
        // set the Node's symmetric socket to the passed socket
        setSymmetricSocket(matchableSockAddr);
        activateSymmetricSocket();
    }
}

void NetworkPeer::softReset() {
    qCDebug(networking) << "Soft reset ";
    // a soft reset should clear the sockets and reset the number of connection attempts
    _localSocket.clear();
    _publicSocket.clear();
    _symmetricSocket.clear();
    _activeSocket = NULL;

    // stop our ping timer since we don't have sockets to ping anymore anyways
    stopPingTimer();

    _connectionAttempts = 0;
}

void NetworkPeer::reset() {
    softReset();

    _uuid = QUuid();
    _wakeTimestamp = QDateTime::currentMSecsSinceEpoch();
    _lastHeardMicrostamp = usecTimestampNow();
}

QByteArray NetworkPeer::toByteArray() const {
    QByteArray peerByteArray;

    QDataStream peerStream(&peerByteArray, QIODevice::Append);
    peerStream << *this;

    return peerByteArray;
}

void NetworkPeer::startPingTimer() {
    if (!_pingTimer) {
        _pingTimer = new QTimer(this);

        connect(_pingTimer, &QTimer::timeout, this, &NetworkPeer::pingTimerTimeout);

        _pingTimer->start(UDP_PUNCH_PING_INTERVAL_MS);
    }
}

void NetworkPeer::stopPingTimer() {
    if (_pingTimer) {
        _pingTimer->stop();
        _pingTimer->deleteLater();
        _pingTimer = NULL;
    }
}

QDataStream& operator<<(QDataStream& out, const NetworkPeer& peer) {
    out << peer._uuid;
    out << peer._publicSocket;
    out << peer._localSocket;

    return out;
}

QDataStream& operator>>(QDataStream& in, NetworkPeer& peer) {
    in >> peer._uuid;
    in >> peer._publicSocket;
    in >> peer._localSocket;

    return in;
}

QDebug operator<<(QDebug debug, const NetworkPeer &peer) {
    debug << uuidStringWithoutCurlyBraces(peer.getUUID())
        << "- public:" << peer.getPublicSocket()
        << "- local:" << peer.getLocalSocket();
    return debug;
}
