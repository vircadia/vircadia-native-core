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
#include "CongestionControl.h"
#include "Connection.h"

#define UDT_CONNECTION_DEBUG

namespace udt {

class BasePacket;
class ControlSender;
class Packet;
class PacketList;
class SequenceNumber;
class UDTTest;

using PacketFilterOperator = std::function<bool(const Packet&)>;

using BasePacketHandler = std::function<void(std::unique_ptr<BasePacket>)>;
using PacketHandler = std::function<void(std::unique_ptr<Packet>)>;
using PacketListHandler = std::function<void(std::unique_ptr<PacketList>)>;

class Socket : public QObject {
    Q_OBJECT
public:
    Socket(QObject* object = 0);
    
    quint16 localPort() const { return _udpSocket.localPort(); }
    
    // Simple functions writing to the socket with no processing
    qint64 writeBasePacket(const BasePacket& packet, const HifiSockAddr& sockAddr);
    qint64 writePacket(const Packet& packet, const HifiSockAddr& sockAddr);
    qint64 writePacket(std::unique_ptr<Packet> packet, const HifiSockAddr& sockAddr);
    qint64 writePacketList(std::unique_ptr<PacketList> packetList, const HifiSockAddr& sockAddr);
    qint64 writeDatagram(const char* data, qint64 size, const HifiSockAddr& sockAddr);
    qint64 writeDatagram(const QByteArray& datagram, const HifiSockAddr& sockAddr);
    
    void bind(const QHostAddress& address, quint16 port = 0) { _udpSocket.bind(address, port); setSystemBufferSizes(); }
    void rebind();
    
    void setPacketFilterOperator(PacketFilterOperator filterOperator) { _packetFilterOperator = filterOperator; }
    void setPacketHandler(PacketHandler handler) { _packetHandler = handler; }
    void setPacketListHandler(PacketListHandler handler) { _packetListHandler = handler; }
    
    void addUnfilteredHandler(const HifiSockAddr& senderSockAddr, BasePacketHandler handler)
        { _unfilteredHandlers[senderSockAddr] = handler; }
    
    void setCongestionControlFactory(std::unique_ptr<CongestionControlVirtualFactory> ccFactory);

    void messageReceived(std::unique_ptr<PacketList> packetList);

public slots:
    void cleanupConnection(HifiSockAddr sockAddr);
    void clearConnections();
    
private slots:
    void readPendingDatagrams();
    void rateControlSync();
    
private:
    void setSystemBufferSizes();
    Connection& findOrCreateConnection(const HifiSockAddr& sockAddr);
   
    // privatized methods used by UDTTest - they are private since they must be called on the Socket thread
    ConnectionStats::Stats sampleStatsForConnection(const HifiSockAddr& destination);
    std::vector<HifiSockAddr> getConnectionSockAddrs();
    void connectToSendSignal(const HifiSockAddr& destinationAddr, QObject* receiver, const char* slot);
    
    Q_INVOKABLE void writeReliablePacket(Packet* packet, const HifiSockAddr& sockAddr);
    Q_INVOKABLE void writeReliablePacketList(PacketList* packetList, const HifiSockAddr& sockAddr);
    
    QUdpSocket _udpSocket { this };
    PacketFilterOperator _packetFilterOperator;
    PacketHandler _packetHandler;
    PacketListHandler _packetListHandler;
    
    std::unordered_map<HifiSockAddr, BasePacketHandler> _unfilteredHandlers;
    std::unordered_map<HifiSockAddr, SequenceNumber> _unreliableSequenceNumbers;
    std::unordered_map<HifiSockAddr, std::unique_ptr<Connection>> _connectionsHash;
    
    int _synInterval = 10; // 10ms
    QTimer _synTimer;
    
    std::unique_ptr<CongestionControlVirtualFactory> _ccFactory { new CongestionControlFactory<DefaultCC>() };
    
    friend class UDTTest;
};
    
} // namespace udt

#endif // hifi_Socket_h
