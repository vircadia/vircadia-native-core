//
//  Socket.h
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-20.
//  Copyright 2015 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_Socket_h
#define hifi_Socket_h

#include <functional>
#include <unordered_map>
#include <mutex>
#include <list>

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include "../SockAddr.h"
#include "TCPVegasCC.h"
#include "Connection.h"
#include "NetworkSocket.h"

//#define UDT_CONNECTION_DEBUG

class UDTTest;

namespace udt {

class BasePacket;
class Packet;
class PacketList;
class SequenceNumber;

using PacketFilterOperator = std::function<bool(const Packet&)>;
using ConnectionCreationFilterOperator = std::function<bool(const SockAddr&)>;

using BasePacketHandler = std::function<void(std::unique_ptr<BasePacket>)>;
using PacketHandler = std::function<void(std::unique_ptr<Packet>)>;
using MessageHandler = std::function<void(std::unique_ptr<Packet>)>;
using MessageFailureHandler = std::function<void(SockAddr, udt::Packet::MessageNumber)>;

class Socket : public QObject {
    Q_OBJECT

    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    using StatsVector = std::vector<std::pair<SockAddr, ConnectionStats::Stats>>;
 
    Socket(QObject* object = 0, bool shouldChangeSocketOptions = true);
    
    quint16 localPort(SocketType socketType) const { return _networkSocket.localPort(socketType); }
    
    // Simple functions writing to the socket with no processing
    qint64 writeBasePacket(const BasePacket& packet, const SockAddr& sockAddr);
    qint64 writePacket(const Packet& packet, const SockAddr& sockAddr);
    qint64 writePacket(std::unique_ptr<Packet> packet, const SockAddr& sockAddr);
    qint64 writePacketList(std::unique_ptr<PacketList> packetList, const SockAddr& sockAddr);
    qint64 writeDatagram(const char* data, qint64 size, const SockAddr& sockAddr);
    qint64 writeDatagram(const QByteArray& datagram, const SockAddr& sockAddr);
    
    void bind(SocketType socketType, const QHostAddress& address, quint16 port = 0);
    void rebind(SocketType socketType, quint16 port);
    void rebind(SocketType socketType);

    void setPacketFilterOperator(PacketFilterOperator filterOperator) { _packetFilterOperator = filterOperator; }
    void setPacketHandler(PacketHandler handler) { _packetHandler = handler; }
    void setMessageHandler(MessageHandler handler) { _messageHandler = handler; }
    void setMessageFailureHandler(MessageFailureHandler handler) { _messageFailureHandler = handler; }
    void setConnectionCreationFilterOperator(ConnectionCreationFilterOperator filterOperator)
        { _connectionCreationFilterOperator = filterOperator; }
    
    void addUnfilteredHandler(const SockAddr& senderSockAddr, BasePacketHandler handler)
        { _unfilteredHandlers[senderSockAddr] = handler; }
    
    void setCongestionControlFactory(std::unique_ptr<CongestionControlVirtualFactory> ccFactory);
    void setConnectionMaxBandwidth(int maxBandwidth);

    void messageReceived(std::unique_ptr<Packet> packet);
    void messageFailed(Connection* connection, Packet::MessageNumber messageNumber);
    
    StatsVector sampleStatsForAllConnections();

#if defined(WEBRTC_DATA_CHANNELS)
    const WebRTCSocket* getWebRTCSocket();
#endif

#if (PR_BUILD || DEV_BUILD)
    void sendFakedHandshakeRequest(const SockAddr& sockAddr);
#endif

signals:
    void clientHandshakeRequestComplete(const SockAddr& sockAddr);

public slots:
    void cleanupConnection(SockAddr sockAddr);
    void clearConnections();
    void handleRemoteAddressChange(SockAddr previousAddress, SockAddr currentAddress);

private slots:
    void readPendingDatagrams();
    void checkForReadyReadBackup();

    void handleSocketError(SocketType socketType, QAbstractSocket::SocketError socketError);
    void handleStateChanged(SocketType socketType, QAbstractSocket::SocketState socketState);

private:
    void setSystemBufferSizes(SocketType socketType);
    Connection* findOrCreateConnection(const SockAddr& sockAddr, bool filterCreation = false);
   
    // privatized methods used by UDTTest - they are private since they must be called on the Socket thread
    ConnectionStats::Stats sampleStatsForConnection(const SockAddr& destination);
    
    std::vector<SockAddr> getConnectionSockAddrs();
    void connectToSendSignal(const SockAddr& destinationAddr, QObject* receiver, const char* slot);
    
    Q_INVOKABLE void writeReliablePacket(Packet* packet, const SockAddr& sockAddr);
    Q_INVOKABLE void writeReliablePacketList(PacketList* packetList, const SockAddr& sockAddr);
    
    NetworkSocket _networkSocket;
    PacketFilterOperator _packetFilterOperator;
    PacketHandler _packetHandler;
    MessageHandler _messageHandler;
    MessageFailureHandler _messageFailureHandler;
    ConnectionCreationFilterOperator _connectionCreationFilterOperator;

    Mutex _unreliableSequenceNumbersMutex;
    Mutex _connectionsHashMutex;

    std::unordered_map<SockAddr, BasePacketHandler> _unfilteredHandlers;
    std::unordered_map<SockAddr, SequenceNumber> _unreliableSequenceNumbers;
    std::unordered_map<SockAddr, std::unique_ptr<Connection>> _connectionsHash;

    QTimer* _readyReadBackupTimer { nullptr };

    int _maxBandwidth { -1 };

    std::unique_ptr<CongestionControlVirtualFactory> _ccFactory { new CongestionControlFactory<TCPVegasCC>() };

    bool _shouldChangeSocketOptions { true };

    int _lastPacketSizeRead { 0 };
    SequenceNumber _lastReceivedSequenceNumber;
    SockAddr _lastPacketSockAddr;
    
    friend UDTTest;
};
    
} // namespace udt

#endif // hifi_Socket_h
