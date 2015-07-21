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

#include <QtCore/QObject>
#include <QtNetwork/QUdpSocket>

#include "../HifiSockAddr.h"
#include "Packet.h"

namespace udt {

using VerifiedPacketFunction = std::function<void(std::unique_ptr<Packet>)>;

class Socket : public QObject {
    Q_OBJECT
public:
    Socket(QObject* object = 0);
    
    quint16 localPort() const { return _udpSocket.localPort(); }
    
    qint64 writeDatagram(const char* data, qint64 size, const HifiSockAddr& sockAddr)
        { return writeDatagram(QByteArray::fromRawData(data, size), sockAddr); }
    qint64 writeDatagram(const QByteArray& datagram, const HifiSockAddr& sockAddr);
    
    void bind(const QHostAddress& address, quint16 port = 0) { _udpSocket.bind(address, port); }
    void rebind();
    
    void setVerifiedPacketFunction(VerifiedPacketFunction verifiedPacketFunction)
        { _verifiedPacketFunction = verifiedPacketFunction; }
    
    void setBufferSizes(int numBytes);

private slots:
    void readPendingDatagrams();
    
private:
    QUdpSocket _udpSocket { this };
    VerifiedPacketFunction _verifiedPacketFunction;
};
    
}


#endif // hifi_Socket_h
