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

#include <atomic>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QUuid>

#include "HifiSockAddr.h"

const QString ICE_SERVER_HOSTNAME = "localhost";
const int ICE_SERVER_DEFAULT_PORT = 7337;
const int ICE_HEARBEAT_INTERVAL_MSECS = 2 * 1000;
const int MAX_ICE_CONNECTION_ATTEMPTS = 5;

const int UDP_PUNCH_PING_INTERVAL_MS = 25;

class NetworkPeer : public QObject {
    Q_OBJECT
public:
    NetworkPeer(QObject* parent = 0);
    NetworkPeer(const QUuid& uuid, const HifiSockAddr& publicSocket, const HifiSockAddr& localSocket, QObject* parent = 0);

    bool isNull() const { return _uuid.isNull(); }
    bool hasSockets() const { return !_localSocket.isNull() && !_publicSocket.isNull(); }

    const QUuid& getUUID() const { return _uuid; }
    void setUUID(const QUuid& uuid) { _uuid = uuid; }

    void softReset();
    void reset();

    const HifiSockAddr& getPublicSocket() const { return _publicSocket; }
    const HifiSockAddr& getLocalSocket() const { return _localSocket; }
    const HifiSockAddr& getSymmetricSocket() const { return _symmetricSocket; }

    void setPublicSocket(const HifiSockAddr& publicSocket);
    void setLocalSocket(const HifiSockAddr& localSocket);
    void setSymmetricSocket(const HifiSockAddr& symmetricSocket);

    const HifiSockAddr* getActiveSocket() const { return _activeSocket; }

    void activatePublicSocket();
    void activateLocalSocket();
    void activateSymmetricSocket();

    void activateMatchingOrNewSymmetricSocket(const HifiSockAddr& matchableSockAddr);

    quint64 getWakeTimestamp() const { return _wakeTimestamp; }
    void setWakeTimestamp(quint64 wakeTimestamp) { _wakeTimestamp = wakeTimestamp; }

    quint64 getLastHeardMicrostamp() const { return _lastHeardMicrostamp; }
    void setLastHeardMicrostamp(quint64 lastHeardMicrostamp) { _lastHeardMicrostamp = lastHeardMicrostamp; }

    QByteArray toByteArray() const;

    int getConnectionAttempts() const  { return _connectionAttempts; }
    void incrementConnectionAttempts() { ++_connectionAttempts; }
    void resetConnectionAttempts() { _connectionAttempts = 0; }

    void recordBytesSent(int count);
    void recordBytesReceived(int count);

    float getOutboundBandwidth(); // in kbps
    float getInboundBandwidth(); // in kbps

    friend QDataStream& operator<<(QDataStream& out, const NetworkPeer& peer);
    friend QDataStream& operator>>(QDataStream& in, NetworkPeer& peer);
public slots:
    void startPingTimer();
    void stopPingTimer();
signals:
    void pingTimerTimeout();
protected:
    void setActiveSocket(HifiSockAddr* discoveredSocket);

    QUuid _uuid;

    HifiSockAddr _publicSocket;
    HifiSockAddr _localSocket;
    HifiSockAddr _symmetricSocket;
    HifiSockAddr* _activeSocket;

    quint64 _wakeTimestamp;
    std::atomic_ullong _lastHeardMicrostamp;

    QTimer* _pingTimer = NULL;

    int _connectionAttempts;
};

QDebug operator<<(QDebug debug, const NetworkPeer &peer);
typedef QSharedPointer<NetworkPeer> SharedNetworkPeer;

#endif // hifi_NetworkPeer_h
