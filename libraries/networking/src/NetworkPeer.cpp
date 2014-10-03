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

#include <qdatetime.h>

#include <SharedUtil.h>
#include <UUID.h>

#include "NetworkPeer.h"

NetworkPeer::NetworkPeer() :
    _uuid(),
    _publicSocket(),
    _localSocket(),
    _wakeTimestamp(QDateTime::currentMSecsSinceEpoch()),
    _lastHeardMicrostamp(usecTimestampNow()),
    _connectionAttempts(0)
{
    
}

NetworkPeer::NetworkPeer(const QUuid& uuid, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket) :
    _uuid(uuid),
    _publicSocket(publicSocket),
    _localSocket(localSocket),
    _wakeTimestamp(QDateTime::currentMSecsSinceEpoch()),
    _lastHeardMicrostamp(usecTimestampNow()),
    _connectionAttempts(0)
{
    
}

NetworkPeer::NetworkPeer(const NetworkPeer& otherPeer) {
    
    _uuid = otherPeer._uuid;
    _publicSocket = otherPeer._publicSocket;
    _localSocket = otherPeer._localSocket;
    
    _wakeTimestamp = otherPeer._wakeTimestamp;
    _lastHeardMicrostamp = otherPeer._lastHeardMicrostamp;
}

NetworkPeer& NetworkPeer::operator=(const NetworkPeer& otherPeer) {
    NetworkPeer temp(otherPeer);
    swap(temp);
    return *this;
}

void NetworkPeer::swap(NetworkPeer& otherPeer) {
    using std::swap;
    
    swap(_uuid, otherPeer._uuid);
    swap(_publicSocket, otherPeer._publicSocket);
    swap(_localSocket, otherPeer._localSocket);
    swap(_wakeTimestamp, otherPeer._wakeTimestamp);
    swap(_lastHeardMicrostamp, otherPeer._lastHeardMicrostamp);
}

QByteArray NetworkPeer::toByteArray() const {
    QByteArray peerByteArray;

    QDataStream peerStream(&peerByteArray, QIODevice::Append);
    peerStream << *this;
    
    return peerByteArray;
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