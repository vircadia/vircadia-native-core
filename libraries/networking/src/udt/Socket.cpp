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

#include "../NetworkLogging.h"

using namespace udt;

Socket::Socket(QObject* parent) :
    QObject(parent)
{
    connect(&_udpSocket, &QUdpSocket::readyRead, this, &Socket::readPendingDatagrams);
}

void Socket::rebind() {
    quint16 oldPort = _udpSocket.localPort();
    
    _udpSocket.close();
    _udpSocket.bind(QHostAddress::AnyIPv4, oldPort);
}

void Socket::setBufferSizes(int numBytes) {
    for (int i = 0; i < 2; i++) {
        QAbstractSocket::SocketOption bufferOpt;
        QString bufferTypeString;
        
        if (i == 0) {
            bufferOpt = QAbstractSocket::SendBufferSizeSocketOption;
            bufferTypeString = "send";
            
        } else {
            bufferOpt = QAbstractSocket::ReceiveBufferSizeSocketOption;
            bufferTypeString = "receive";
        }
        
        int oldBufferSize = _udpSocket.socketOption(bufferOpt).toInt();
        
        if (oldBufferSize < numBytes) {
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

qint64 Socket::writeUnreliablePacket(const Packet& packet, const HifiSockAddr& sockAddr) {
    return writeDatagram(packet.getData(), packet.getDataSize(), sockAddr);
}

qint64 Socket::writeDatagram(const QByteArray& datagram, const HifiSockAddr& sockAddr) {
    
    qint64 bytesWritten = _udpSocket.writeDatagram(datagram, sockAddr.getAddress(), sockAddr.getPort());
    
    // TODO::
    // const_cast<NLPacket&>(packet).writeSequenceNumber(sequenceNumber);
    
    if (bytesWritten < 0) {
        qCDebug(networking) << "ERROR in writeDatagram:" << _udpSocket.error() << "-" << _udpSocket.errorString();
    }
    
    return bytesWritten;
}

void Socket::readPendingDatagrams() {
    while (_udpSocket.hasPendingDatagrams()) {
        // setup a HifiSockAddr to read into
        HifiSockAddr senderSockAddr;
        
        // setup a buffer to read the packet into
        int packetSizeWithHeader = _udpSocket.pendingDatagramSize();
        std::unique_ptr<char> buffer = std::unique_ptr<char>(new char[packetSizeWithHeader]);
       
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
        
        // setup a Packet from the data we just read
        auto packet = Packet::fromReceivedPacket(std::move(buffer), packetSizeWithHeader, senderSockAddr);
        
        // call our verification operator to see if this packet is verified
        if (!_packetFilterOperator || _packetFilterOperator(*packet)) {
            if (_packetHandler) {
                // call the verified packet callback to let it handle this packet
                return _packetHandler(std::move(packet));
            }
        }
    }
}
