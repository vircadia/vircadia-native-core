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
#include "BandwidthRecorder.h"

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

NetworkPeer::NetworkPeer(const NetworkPeer& otherPeer) : QObject() {
    _uuid = otherPeer._uuid;
    _publicSocket = otherPeer._publicSocket;
    _localSocket = otherPeer._localSocket;
    
    _wakeTimestamp = otherPeer._wakeTimestamp;
    _lastHeardMicrostamp = otherPeer._lastHeardMicrostamp;
    _connectionAttempts = otherPeer._connectionAttempts;
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
    swap(_connectionAttempts, otherPeer._connectionAttempts);
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


// FIXME this is a temporary implementation to determine if this is the right approach.
// If so, migrate the BandwidthRecorder into the NetworkPeer class
using BandwidthRecorderPtr = QSharedPointer<BandwidthRecorder>;
static QHash<QUuid, BandwidthRecorderPtr> PEER_BANDWIDTH;

BandwidthRecorder& getBandwidthRecorder(const QUuid & uuid) {
    if (!PEER_BANDWIDTH.count(uuid)) {
        PEER_BANDWIDTH.insert(uuid, BandwidthRecorderPtr(new BandwidthRecorder()));
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
