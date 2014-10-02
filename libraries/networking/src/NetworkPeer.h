//
//  NetworkPeer.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2014-10-02.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkPeer_h
#define hifi_NetworkPeer_h

#include <qobject.h>
#include <quuid.h>

#include "HifiSockAddr.h"

const QString ICE_SERVER_HOSTNAME = "localhost";
const int ICE_SERVER_DEFAULT_PORT = 7337;
const int ICE_HEARBEAT_INTERVAL_MSECS = 2 * 1000;

class NetworkPeer : public QObject {
public:
    NetworkPeer(const QUuid& uuid, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket);
    
    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid) { _uuid = uuid; }
    
    const HifiSockAddr& getPublicSocket() const { return _publicSocket; }
    void setPublicSocket(const HifiSockAddr& publicSocket);
    const HifiSockAddr& getLocalSocket() const { return _localSocket; }
    void setLocalSocket(const HifiSockAddr& localSocket);
    const HifiSockAddr& getSymmetricSocket() const { return _symmetricSocket; }
    void setSymmetricSocket(const HifiSockAddr& symmetricSocket);
    
    const HifiSockAddr* getActiveSocket() const { return _activeSocket; }
    
    void activatePublicSocket();
    void activateLocalSocket();
    void activateSymmetricSocket();
    
    quint64 getWakeTimestamp() const { return _wakeTimestamp; }
    void setWakeTimestamp(quint64 wakeTimestamp) { _wakeTimestamp = wakeTimestamp; }
    
    quint64 getLastHeardMicrostamp() const { return _lastHeardMicrostamp; }
    void setLastHeardMicrostamp(quint64 lastHeardMicrostamp) { _lastHeardMicrostamp = lastHeardMicrostamp; }
    
    QByteArray toByteArray() const;
    
    friend QDataStream& operator<<(QDataStream& out, const NetworkPeer& peer);
    friend QDataStream& operator>>(QDataStream& in, NetworkPeer& peer);
protected:
    QUuid _uuid;
    
    HifiSockAddr _publicSocket;
    HifiSockAddr _localSocket;
    HifiSockAddr _symmetricSocket;
    HifiSockAddr* _activeSocket;
    
    quint64 _wakeTimestamp;
    quint64 _lastHeardMicrostamp;
};

QDebug operator<<(QDebug debug, const NetworkPeer &peer);
typedef QSharedPointer<NetworkPeer> SharedNetworkPeer;

#endif // hifi_NetworkPeer_h