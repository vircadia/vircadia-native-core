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

#ifdef Q_OS_ANDROID
#include <sys/socket.h>
#endif

#include <QtCore/QThread>

#include <LogHandler.h>

#include "../NetworkLogging.h"
#include "Connection.h"
#include "ControlPacket.h"
#include "Packet.h"
#include "../NLPacket.h"
#include "../NLPacketList.h"
#include "PacketList.h"

using namespace udt;

Socket::Socket(QObject* parent) :
    QObject(parent),
    _synTimer(new QTimer(this))
{
    connect(&_udpSocket, &QUdpSocket::readyRead, this, &Socket::readPendingDatagrams);

    // make sure our synchronization method is called every SYN interval
    connect(_synTimer, &QTimer::timeout, this, &Socket::rateControlSync);

    // start our timer for the synchronization time interval
    _synTimer->start(_synInterval);

    // make sure we hear about errors and state changes from the underlying socket
    connect(&_udpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(handleSocketError(QAbstractSocket::SocketError)));
    connect(&_udpSocket, &QAbstractSocket::stateChanged, this, &Socket::handleStateChanged);
}

void Socket::bind(const QHostAddress& address, quint16 port) {
    _udpSocket.bind(address, port);
    setSystemBufferSizes();

#if defined(Q_OS_LINUX)
    auto sd = _udpSocket.socketDescriptor();
    int val = IP_PMTUDISC_DONT;
    setsockopt(sd, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));
#elif defined(Q_OS_WINDOWS)
    auto sd = _udpSocket.socketDescriptor();
    int val = 0; // false
    setsockopt(sd, IPPROTO_IP, IP_DONTFRAGMENT, &val, sizeof(val));
#endif
}

void Socket::rebind() {
    rebind(_udpSocket.localPort());
}

void Socket::rebind(quint16 localPort) {
    _udpSocket.close();
    bind(QHostAddress::AnyIPv4, localPort);
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
            auto ptr = packetList.release();
            QMetaObject::invokeMethod(this, "writeReliablePacketList", Qt::AutoConnection,
                                      Q_ARG(PacketList*, ptr),
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
    auto connection = findOrCreateConnection(sockAddr);
    if (connection) {
        connection->sendReliablePacket(std::unique_ptr<Packet>(packet));
    }
#ifdef UDT_CONNECTION_DEBUG
    else {
        qCDebug(networking) << "Socket::writeReliablePacket refusing to send packet - no connection was created";
    }
#endif

}

void Socket::writeReliablePacketList(PacketList* packetList, const HifiSockAddr& sockAddr) {
    auto connection = findOrCreateConnection(sockAddr);
    if (connection) {
        connection->sendReliablePacketList(std::unique_ptr<PacketList>(packetList));
    }
#ifdef UDT_CONNECTION_DEBUG
    else {
        qCDebug(networking) << "Socket::writeReliablePacketList refusing to send packet list - no connection was created";
    }
#endif
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

Connection* Socket::findOrCreateConnection(const HifiSockAddr& sockAddr) {
    auto it = _connectionsHash.find(sockAddr);

    if (it == _connectionsHash.end()) {
        // we did not have a matching connection, time to see if we should make one

        if (_connectionCreationFilterOperator && !_connectionCreationFilterOperator(sockAddr)) {
            // the connection creation filter did not allow us to create a new connection
#ifdef UDT_CONNECTION_DEBUG
            qCDebug(networking) << "Socket::findOrCreateConnection refusing to create connection for" << sockAddr
                << "due to connection creation filter";
#endif
            return nullptr;
        } else {
            auto congestionControl = _ccFactory->create();
            congestionControl->setMaxBandwidth(_maxBandwidth);
            auto connection = std::unique_ptr<Connection>(new Connection(this, sockAddr, std::move(congestionControl)));

            // we queue the connection to cleanup connection in case it asks for it during its own rate control sync
            QObject::connect(connection.get(), &Connection::connectionInactive, this, &Socket::cleanupConnection);

            // allow higher-level classes to find out when connections have completed a handshake
            QObject::connect(connection.get(), &Connection::receiverHandshakeRequestComplete,
                             this, &Socket::clientHandshakeRequestComplete);

#ifdef UDT_CONNECTION_DEBUG
            qCDebug(networking) << "Creating new connection to" << sockAddr;
#endif

            it = _connectionsHash.insert(it, std::make_pair(sockAddr, std::move(connection)));
        }
    }

    return it->second.get();
}

void Socket::clearConnections() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "clearConnections", Qt::BlockingQueuedConnection);
        return;
    }

    if (_connectionsHash.size() > 0) {
        // clear all of the current connections in the socket
        qDebug() << "Clearing all remaining connections in Socket.";
        _connectionsHash.clear();
    }
}

void Socket::cleanupConnection(HifiSockAddr sockAddr) {
    auto numErased = _connectionsHash.erase(sockAddr);

    if (numErased > 0) {
#ifdef UDT_CONNECTION_DEBUG
        qCDebug(networking) << "Socket::cleanupConnection called for UDT connection to" << sockAddr;
#endif
    }
}

void Socket::messageReceived(std::unique_ptr<Packet> packet) {
    if (_messageHandler) {
        _messageHandler(std::move(packet));
    }
}

void Socket::messageFailed(Connection* connection, Packet::MessageNumber messageNumber) {
    if (_messageFailureHandler) {
        _messageFailureHandler(connection->getDestination(), messageNumber);
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
        auto sizeRead = _udpSocket.readDatagram(buffer.get(), packetSizeWithHeader,
                                                senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());

        if (sizeRead <= 0) {
            // we either didn't pull anything for this packet or there was an error reading (this seems to trigger
            // on windows even if there's not a packet available)
            continue;
        }

        auto it = _unfilteredHandlers.find(senderSockAddr);

        if (it != _unfilteredHandlers.end()) {
            // we have a registered unfiltered handler for this HifiSockAddr - call that and return
            if (it->second) {
                auto basePacket = BasePacket::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);
                it->second(std::move(basePacket));
            }

            continue;
        }

        // check if this was a control packet or a data packet
        bool isControlPacket = *reinterpret_cast<uint32_t*>(buffer.get()) & CONTROL_BIT_MASK;

        if (isControlPacket) {
            // setup a control packet from the data we just read
            auto controlPacket = ControlPacket::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);

            // move this control packet to the matching connection, if there is one
            auto connection = findOrCreateConnection(senderSockAddr);

            if (connection) {
                connection->processControl(move(controlPacket));
            }

        } else {
            // setup a Packet from the data we just read
            auto packet = Packet::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);

            // call our verification operator to see if this packet is verified
            if (!_packetFilterOperator || _packetFilterOperator(*packet)) {
                if (packet->isReliable()) {
                    // if this was a reliable packet then signal the matching connection with the sequence number
                    auto connection = findOrCreateConnection(senderSockAddr);

                    if (!connection || !connection->processReceivedSequenceNumber(packet->getSequenceNumber(),
                                                                                  packet->getDataSize(),
                                                                                  packet->getPayloadSize())) {
                        // the connection could not be created or indicated that we should not continue processing this packet
                        continue;
                    }
                }

                if (packet->isPartOfMessage()) {
                    auto connection = findOrCreateConnection(senderSockAddr);
                    if (connection) {
                        connection->queueReceivedMessagePacket(std::move(packet));
                    }
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

    // the way we do this is a little funny looking - we need to avoid the case where we call sync and
    // (because of our Qt direct connection to the Connection's signal that it has been deactivated)
    // an iterator on _connectionsHash would be invalidated by our own call to cleanupConnection

    // collect the sockets for all connections in a vector

    std::vector<HifiSockAddr> sockAddrVector;
    sockAddrVector.reserve(_connectionsHash.size());

    for (auto& connection : _connectionsHash) {
        sockAddrVector.emplace_back(connection.first);
    }

    // enumerate that vector of HifiSockAddr objects
    for (auto& sockAddr : sockAddrVector) {
        // pull out the respective connection via a quick find on the unordered_map
        auto it = _connectionsHash.find(sockAddr);

        if (it != _connectionsHash.end()) {
            // if the connection is erased while calling sync since we are re-using the iterator that was invalidated
            // we're good to go
            auto& connection = _connectionsHash[sockAddr];
            connection->sync();
        }
    }

    if (_synTimer->interval() != _synInterval) {
        // if the _synTimer interval doesn't match the current _synInterval (changes when the CC factory is changed)
        // then restart it now with the right interval
        _synTimer->start(_synInterval);
    }
}

void Socket::setCongestionControlFactory(std::unique_ptr<CongestionControlVirtualFactory> ccFactory) {
    // swap the current unique_ptr for the new factory
    _ccFactory.swap(ccFactory);

    // update the _synInterval to the value from the factory
    _synInterval = _ccFactory->synInterval();
}


void Socket::setConnectionMaxBandwidth(int maxBandwidth) {
    qInfo() << "Setting socket's maximum bandwith to" << maxBandwidth << "bps. ("
            << _connectionsHash.size() << "live connections)";
    _maxBandwidth = maxBandwidth;
    for (auto& pair : _connectionsHash) {
        auto& connection = pair.second;
        connection->setMaxBandwidth(_maxBandwidth);
    }
}

ConnectionStats::Stats Socket::sampleStatsForConnection(const HifiSockAddr& destination) {
    auto it = _connectionsHash.find(destination);
    if (it != _connectionsHash.end()) {
        return it->second->sampleStats();
    } else {
        return ConnectionStats::Stats();
    }
}

Socket::StatsVector Socket::sampleStatsForAllConnections() {
    StatsVector result;
    result.reserve(_connectionsHash.size());
    for (const auto& connectionPair : _connectionsHash) {
        result.emplace_back(connectionPair.first, connectionPair.second->sampleStats());
    }
    return result;
}


std::vector<HifiSockAddr> Socket::getConnectionSockAddrs() {
    std::vector<HifiSockAddr> addr;
    addr.reserve(_connectionsHash.size());

    for (const auto& connectionPair : _connectionsHash) {
        addr.push_back(connectionPair.first);
    }
    return addr;
}

void Socket::handleSocketError(QAbstractSocket::SocketError socketError) {
    qCWarning(networking) << "udt::Socket error -" << socketError;
}

void Socket::handleStateChanged(QAbstractSocket::SocketState socketState) {
    if (socketState != QAbstractSocket::BoundState) {
        qCWarning(networking) << "udt::Socket state changed - state is now" << socketState;
    }
}
