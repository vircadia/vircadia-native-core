//
//  Socket.h
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-20.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_Socket_h
#define hifi_Socket_h

#include <functional>
#include <unordered_map>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtNetwork/QUdpSocket>

#include "../HifiSockAddr.h"
#include "Connection.h"

namespace udt {

class BasePacket;
class ControlSender;
class Packet;
class SequenceNumber;

using PacketFilterOperator = std::function<bool(const Packet&)>;

using BasePacketHandler = std::function<void(std::unique_ptr<BasePacket>)>;
using PacketHandler = std::function<void(std::unique_ptr<Packet>)>;

class Socket : public QObject {
    Q_OBJECT
public:
    Socket(QObject* object = 0);
    
    quint16 localPort() const { return _udpSocket.localPort(); }
    
    // Simple functions writing to the socket with no processing
    qint64 writePacket(const Packet& packet, const HifiSockAddr& sockAddr);
    qint64 writePacket(std::unique_ptr<Packet> packet, const HifiSockAddr& sockAddr);
    qint64 writeDatagram(const char* data, qint64 size, const HifiSockAddr& sockAddr);
    qint64 writeDatagram(const QByteArray& datagram, const HifiSockAddr& sockAddr);
    
    void bind(const QHostAddress& address, quint16 port = 0) { _udpSocket.bind(address, port); }
    void rebind();
    
    void setPacketFilterOperator(PacketFilterOperator filterOperator) { _packetFilterOperator = filterOperator; }
    void setPacketHandler(PacketHandler handler) { _packetHandler = handler; }
    
    void setBufferSizes(int numBytes);
    
    void addUnfilteredHandler(const HifiSockAddr& senderSockAddr, BasePacketHandler handler)
        { _unfilteredHandlers[senderSockAddr] = handler; }

private slots:
    void readPendingDatagrams();
    void rateControlSync();
    
private:
    QUdpSocket _udpSocket { this };
    PacketFilterOperator _packetFilterOperator;
    PacketHandler _packetHandler;
    
    SequenceNumber _currentUnreliableSequenceNumber;
    
    std::unordered_map<HifiSockAddr, BasePacketHandler> _unfilteredHandlers;
    std::unordered_map<HifiSockAddr, Connection*> _connectionsHash;
    
    int32_t _synInterval = 10; // 10ms
    QTimer _synTimer;
};
    
} // namespace udt

#endif // hifi_Socket_h
