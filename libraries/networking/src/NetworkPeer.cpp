//
//  NetworkPeer.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-10-02.
//  Copyright 2014 High Fidelity, Inc.
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

#include "BandwidthRecorder.h"
#include "NetworkLogging.h"


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

NetworkPeer::NetworkPeer(const QUuid& uuid, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket, QObject* parent) :
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

void NetworkPeer::setPublicSocket(const HifiSockAddr& publicSocket) {
    if (publicSocket != _publicSocket) {
        if (_activeSocket == &_publicSocket) {
            // if the active socket was the public socket then reset it to NULL
            _activeSocket = NULL;
        }
        
        bool wasOldSocketNull = _publicSocket.isNull();

        auto temp = _publicSocket.objectName();
        _publicSocket = publicSocket;
        _publicSocket.setObjectName(temp);
        
        if (!wasOldSocketNull) {
            qCDebug(networking) << "Public socket change for node" << *this;
        }
    }
}

void NetworkPeer::setLocalSocket(const HifiSockAddr& localSocket) {
    if (localSocket != _localSocket) {
        if (_activeSocket == &_localSocket) {
            // if the active socket was the local socket then reset it to NULL
            _activeSocket = NULL;
        }
        
        bool wasOldSocketNull = _localSocket.isNull();
        
        auto temp = _localSocket.objectName();
        _localSocket = localSocket;
        _localSocket.setObjectName(temp);

        if (!wasOldSocketNull) {
            qCDebug(networking) << "Local socket change for node" << *this;
        }
    }
}

void NetworkPeer::setSymmetricSocket(const HifiSockAddr& symmetricSocket) {
    if (symmetricSocket != _symmetricSocket) {
        if (_activeSocket == &_symmetricSocket) {
            // if the active socket was the symmetric socket then reset it to NULL
            _activeSocket = NULL;
        }
        
        bool wasOldSocketNull = _symmetricSocket.isNull();
        
        auto temp = _symmetricSocket.objectName();
        _symmetricSocket = symmetricSocket;
        _symmetricSocket.setObjectName(temp);
        
        if (!wasOldSocketNull) {
            qCDebug(networking) << "Symmetric socket change for node" << *this;
        }
    }
}

void NetworkPeer::setActiveSocket(HifiSockAddr* discoveredSocket) {
    _activeSocket = discoveredSocket;

    // we have an active socket, stop our ping timer
    stopPingTimer();

    // we're now considered connected to this peer - reset the number of connection attemps
    resetConnectionAttempts();
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
        qCDebug(networking) << "Activating symmetric socket for network peer with ID" << uuidStringWithoutCurlyBraces(_uuid);
        setActiveSocket(&_symmetricSocket);
    }
}

void NetworkPeer::activateMatchingOrNewSymmetricSocket(const HifiSockAddr& matchableSockAddr) {
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


// FIXME this is a temporary implementation to determine if this is the right approach.
// If so, migrate the BandwidthRecorder into the NetworkPeer class
using BandwidthRecorderPtr = QSharedPointer<BandwidthRecorder>;
static QHash<QUuid, BandwidthRecorderPtr> PEER_BANDWIDTH;

BandwidthRecorder& getBandwidthRecorder(const QUuid & uuid) {
    if (!PEER_BANDWIDTH.count(uuid)) {
        PEER_BANDWIDTH.insert(uuid, QSharedPointer<BandwidthRecorder>::create());
    }
    return *PEER_BANDWIDTH[uuid].data();
}

void NetworkPeer::recordBytesSent(int count) {
    auto& bw = getBandwidthRecorder(_uuid);
    bw.updateOutboundData(0, count);
}

void NetworkPeer::recordBytesReceived(int count) {
    auto& bw = getBandwidthRecorder(_uuid);
    bw.updateInboundData(0, count);
}

float NetworkPeer::getOutboundBandwidth() {
    auto& bw = getBandwidthRecorder(_uuid);
    return bw.getAverageOutputKilobitsPerSecond(0);
}

float NetworkPeer::getInboundBandwidth() {
    auto& bw = getBandwidthRecorder(_uuid);
    return bw.getAverageInputKilobitsPerSecond(0);
}
