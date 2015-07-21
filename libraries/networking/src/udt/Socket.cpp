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

qint64 Socket::writeDatagram(const QByteArray& datagram, const HifiSockAddr& sockAddr) {
    qint64 bytesWritten = _udpSocket.writeDatagram(datagram, sockAddr.getAddress(), sockAddr.getPort());
    
    if (bytesWritten < 0) {
        qCDebug(networking) << "ERROR in writeDatagram:" << _udpSocket.error() << "-" << _udpSocket.errorString();
    }
    
    return bytesWritten;
}
