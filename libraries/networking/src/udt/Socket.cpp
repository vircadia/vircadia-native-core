//
//  Socket.cpp
//  libraries/networking/src/udt
//
//  Created by Stephen Birarda on 2015-07-20.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Socket.h"

#include <QtCore/QThread>

#include <LogHandler.h>

#include "../NetworkLogging.h"
#include "Connection.h"
#include "ControlPacket.h"
#include "Packet.h"
#include "../NLPacket.h"
#include "PacketList.h"

using namespace udt;

Q_DECLARE_METATYPE(Packet*);
Q_DECLARE_METATYPE(PacketList*);

Socket::Socket(QObject* parent) :
    QObject(parent)
{
    qRegisterMetaType<Packet*>();
    qRegisterMetaType<PacketList*>();
    
    connect(&_udpSocket, &QUdpSocket::readyRead, this, &Socket::readPendingDatagrams);
    
    // make sure our synchronization method is called every SYN interval
    connect(&_synTimer, &QTimer::timeout, this, &Socket::rateControlSync);
    
    // start our timer for the synchronization time interval
    _synTimer.start(_synInterval);
}

void Socket::rebind() {
    quint16 oldPort = _udpSocket.localPort();
    
    _udpSocket.close();
    bind(QHostAddress::AnyIPv4, oldPort);
}

void Socket::setSystemBufferSizes() {
    for (int i = 0; i < 2; i++) {
        QAbstractSocket::SocketOption bufferOpt;
        QString bufferTypeString;
        
        int numBytes = 0;
        
        if (i == 0) {
            bufferOpt = QAbstractSocket::SendBufferSizeSocketOption;
            numBytes = udt::UDP_SEND_BUFFER_SIZE_BYTES;
            bufferTypeString = "send";
            
        } else {
            bufferOpt = QAbstractSocket::ReceiveBufferSizeSocketOption;
            numBytes = udt::UDP_RECEIVE_BUFFER_SIZE_BYTES;
            bufferTypeString = "receive";
        }
        
        int oldBufferSize = _udpSocket.socketOption(bufferOpt).toInt();
        
        if (oldBufferSize < numBytes) {
            _udpSocket.setSocketOption(bufferOpt, QVariant(numBytes));
            int newBufferSize = _udpSocket.socketOption(bufferOpt).toInt();
            
            qCDebug(networking) << "Changed socket" << bufferTypeString << "buffer size from" << oldBufferSize << "to"
                << newBufferSize << "bytes";
        } else {
            // don't make the buffer smaller
            qCDebug(networking) << "Did not change socket" << bufferTypeString << "buffer size from" << oldBufferSize
                << "since it is larger than desired size of" << numBytes;
        }
    }
}

qint64 Socket::writeBasePacket(const udt::BasePacket& packet, const HifiSockAddr &sockAddr) {
    // Since this is a base packet we have no way to know if this is reliable or not - we just fire it off
    
    // this should not be called with an instance of Packet
    Q_ASSERT_X(!dynamic_cast<const Packet*>(&packet),
               "Socket::writeBasePacket", "Cannot send a Packet/NLPacket via writeBasePacket");
    
    return writeDatagram(packet.getData(), packet.getDataSize(), sockAddr);
}

qint64 Socket::writePacket(const Packet& packet, const HifiSockAddr& sockAddr) {
    Q_ASSERT_X(!packet.isReliable(), "Socket::writePacket", "Cannot send a reliable packet unreliably");
    
    // write the correct sequence number to the Packet here
    packet.writeSequenceNumber(++_unreliableSequenceNumbers[sockAddr]);
    
    return writeDatagram(packet.getData(), packet.getDataSize(), sockAddr);
}

qint64 Socket::writePacket(std::unique_ptr<Packet> packet, const HifiSockAddr& sockAddr) {
    
    if (packet->isReliable()) {
        // hand this packet off to writeReliablePacket
        // because Qt can't invoke with the unique_ptr we have to release it here and re-construct in writeReliablePacket
        
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "writeReliablePacket", Qt::QueuedConnection,
                                      Q_ARG(Packet*, packet.release()),
                                      Q_ARG(HifiSockAddr, sockAddr));
        } else {
            writeReliablePacket(packet.release(), sockAddr);
        }
        
        return 0;
    }
    
    return writePacket(*packet, sockAddr);
}

qint64 Socket::writePacketList(std::unique_ptr<PacketList> packetList, const HifiSockAddr& sockAddr) {
    if (packetList->isReliable()) {
        // hand this packetList off to writeReliablePacketList
        // because Qt can't invoke with the unique_ptr we have to release it here and re-construct in writeReliablePacketList
        
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "writeReliablePacketList", Qt::QueuedConnection,
                                      Q_ARG(PacketList*, packetList.release()),
                                      Q_ARG(HifiSockAddr, sockAddr));
        } else {
            writeReliablePacketList(packetList.release(), sockAddr);
        }
        
        return 0;
    }

    // Unerliable and Unordered
    qint64 totalBytesSent = 0;
    while (!packetList->_packets.empty()) {
        totalBytesSent += writePacket(packetList->takeFront<Packet>(), sockAddr);
    }

    return totalBytesSent;
}

void Socket::writeReliablePacket(Packet* packet, const HifiSockAddr& sockAddr) {
    findOrCreateConnection(sockAddr).sendReliablePacket(std::unique_ptr<Packet>(packet));
}

void Socket::writeReliablePacketList(PacketList* packetList, const HifiSockAddr& sockAddr) {
    findOrCreateConnection(sockAddr).sendReliablePacketList(std::unique_ptr<PacketList>(packetList));
}

qint64 Socket::writeDatagram(const char* data, qint64 size, const HifiSockAddr& sockAddr) {
    return writeDatagram(QByteArray::fromRawData(data, size), sockAddr);
}

qint64 Socket::writeDatagram(const QByteArray& datagram, const HifiSockAddr& sockAddr) {
    
    qint64 bytesWritten = _udpSocket.writeDatagram(datagram, sockAddr.getAddress(), sockAddr.getPort());
    
    if (bytesWritten < 0) {
        // when saturating a link this isn't an uncommon message - suppress it so it doesn't bomb the debug
        static const QString WRITE_ERROR_REGEX = "Socket::writeDatagram QAbstractSocket::NetworkError - Unable to send a message";
        static QString repeatedMessage
            = LogHandler::getInstance().addRepeatedMessageRegex(WRITE_ERROR_REGEX);
        
        qCDebug(networking) << "Socket::writeDatagram" << _udpSocket.error() << "-" << qPrintable(_udpSocket.errorString());
    }
    
    return bytesWritten;
}

Connection& Socket::findOrCreateConnection(const HifiSockAddr& sockAddr) {
    auto it = _connectionsHash.find(sockAddr);

    if (it == _connectionsHash.end()) {
        auto connection = std::unique_ptr<Connection>(new Connection(this, sockAddr, _ccFactory->create()));
        
        // we queue the connection to cleanup connection in case it asks for it during its own rate control sync
        QObject::connect(connection.get(), &Connection::connectionInactive, this, &Socket::cleanupConnection,
                         Qt::QueuedConnection);
        
        #ifdef UDT_CONNECTION_DEBUG
        qCDebug(networking) << "Creating new connection to" << sockAddr;
        #endif
        
        it = _connectionsHash.insert(it, std::make_pair(sockAddr, std::move(connection)));
    }
    
    return *it->second;
}

void Socket::clearConnections() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearConnections", Qt::BlockingQueuedConnection);
        return;
    }
    
    // clear all of the current connections in the socket
    qDebug() << "Clearing all remaining connections in Socket.";
    _connectionsHash.clear();
}

void Socket::cleanupConnection(HifiSockAddr sockAddr) {
    #ifdef UDT_CONNECTION_DEBUG
    qCDebug(networking) << "Socket::cleanupConnection called for UDT connection to" << sockAddr;
    #endif
    
    _connectionsHash.erase(sockAddr);
}

void Socket::messageReceived(std::unique_ptr<PacketList> packetList) {
    if (_packetListHandler) {
        _packetListHandler(std::move(packetList));
    }
}

void Socket::readPendingDatagrams() {
    int packetSizeWithHeader = -1;
    while ((packetSizeWithHeader = _udpSocket.pendingDatagramSize()) != -1) {
        // setup a HifiSockAddr to read into
        HifiSockAddr senderSockAddr;
        
        // setup a buffer to read the packet into
        auto buffer = std::unique_ptr<char[]>(new char[packetSizeWithHeader]);
       
        // pull the datagram
        _udpSocket.readDatagram(buffer.get(), packetSizeWithHeader,
                                senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        auto it = _unfilteredHandlers.find(senderSockAddr);
        
        if (it != _unfilteredHandlers.end()) {
            // we have a registered unfiltered handler for this HifiSockAddr - call that and return
            if (it->second) {
                auto basePacket = BasePacket::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);
                it->second(std::move(basePacket));
            }
            
            return;
        }
        
        // check if this was a control packet or a data packet
        bool isControlPacket = *reinterpret_cast<uint32_t*>(buffer.get()) & CONTROL_BIT_MASK;
        
        if (isControlPacket) {
            // setup a control packet from the data we just read
            auto controlPacket = ControlPacket::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);
            
            // move this control packet to the matching connection
            auto& connection = findOrCreateConnection(senderSockAddr);
            connection.processControl(move(controlPacket));
            
        } else {
            // setup a Packet from the data we just read
            auto packet = Packet::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);
            
            // call our verification operator to see if this packet is verified
            if (!_packetFilterOperator || _packetFilterOperator(*packet)) {
                if (packet->isReliable()) {
                    // if this was a reliable packet then signal the matching connection with the sequence number
                    auto& connection = findOrCreateConnection(senderSockAddr);
                    
                    if (!connection.processReceivedSequenceNumber(packet->getSequenceNumber(),
                                                                  packet->getDataSize(),
                                                                  packet->getPayloadSize())) {
                        // the connection indicated that we should not continue processing this packet
                        return;
                    }
                }

                if (packet->isPartOfMessage()) {
                    auto& connection = findOrCreateConnection(senderSockAddr);
                    connection.queueReceivedMessagePacket(std::move(packet));
                } else if (_packetHandler) {
                    // call the verified packet callback to let it handle this packet
                    _packetHandler(std::move(packet));
                }
            }
        }
    }
}

void Socket::connectToSendSignal(const HifiSockAddr& destinationAddr, QObject* receiver, const char* slot) {
    auto it = _connectionsHash.find(destinationAddr);
    if (it != _connectionsHash.end()) {
        connect(it->second.get(), SIGNAL(packetSent()), receiver, slot);
    }
}

void Socket::rateControlSync() {
    
    // enumerate our list of connections and ask each of them to send off periodic ACK packet for rate control
    for (auto& connection : _connectionsHash) {
        connection.second->sync();
    }
    
    if (_synTimer.interval() != _synInterval) {
        // if the _synTimer interval doesn't match the current _synInterval (changes when the CC factory is changed)
        // then restart it now with the right interval
        _synTimer.start(_synInterval);
    }
}

void Socket::setCongestionControlFactory(std::unique_ptr<CongestionControlVirtualFactory> ccFactory) {
    // swap the current unique_ptr for the new factory
    _ccFactory.swap(ccFactory);
    
    // update the _synInterval to the value from the factory
    _synInterval = _ccFactory->synInterval();
}

ConnectionStats::Stats Socket::sampleStatsForConnection(const HifiSockAddr& destination) {
    auto it = _connectionsHash.find(destination);
    if (it != _connectionsHash.end()) {
        return it->second->sampleStats();
    } else {
        return ConnectionStats::Stats();
    }
}

std::vector<HifiSockAddr> Socket::getConnectionSockAddrs() {    
    std::vector<HifiSockAddr> addr;
    addr.reserve(_connectionsHash.size());
    
    for (const auto& connectionPair : _connectionsHash) {
        addr.push_back(connectionPair.first);
    }
    return addr;
}
