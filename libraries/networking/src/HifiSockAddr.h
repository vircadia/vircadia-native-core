//
//  HifiSockAddr.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 11/26/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_HifiSockAddr_h
#define hifi_HifiSockAddr_h

#ifdef WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include <qhostaddress.h>
#include <qhostinfo.h>

class HifiSockAddr : public QObject {
    Q_OBJECT
public:
    HifiSockAddr();
    HifiSockAddr(const QHostAddress& address, quint16 port);
    HifiSockAddr(const HifiSockAddr& otherSockAddr);
    HifiSockAddr(const QString& hostname, quint16 hostOrderPort);
    HifiSockAddr(const sockaddr* sockaddr);
    
    bool isNull() const { return _address.isNull() && _port == 0; }

    HifiSockAddr& operator=(const HifiSockAddr& rhsSockAddr);
    void swap(HifiSockAddr& otherSockAddr);
    
    bool operator==(const HifiSockAddr& rhsSockAddr) const;
    bool operator!=(const HifiSockAddr& rhsSockAddr) const { return !(*this == rhsSockAddr); }
    
    const QHostAddress& getAddress() const { return _address; }
    QHostAddress* getAddressPointer() { return &_address; }
    void setAddress(const QHostAddress& address) { _address = address; }
    
    quint16 getPort() const { return _port; }
    quint16* getPortPointer() { return &_port; }
    void setPort(quint16 port) { _port = port; }
    
    static int packSockAddr(unsigned char* packetData, const HifiSockAddr& packSockAddr);
    static int unpackSockAddr(const unsigned char* packetData, HifiSockAddr& unpackDestSockAddr);
    
    friend QDebug operator<<(QDebug debug, const HifiSockAddr& sockAddr);
    friend QDataStream& operator<<(QDataStream& dataStream, const HifiSockAddr& sockAddr);
    friend QDataStream& operator>>(QDataStream& dataStream, HifiSockAddr& sockAddr);
private slots:
    void handleLookupResult(const QHostInfo& hostInfo);
private:
    QHostAddress _address;
    quint16 _port;
};

uint qHash(const HifiSockAddr& key, uint seed);

QHostAddress getLocalAddress();

Q_DECLARE_METATYPE(HifiSockAddr)

#endif // hifi_HifiSockAddr_h
