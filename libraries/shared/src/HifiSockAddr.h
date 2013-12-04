//
//  HifiSockAddr.h
//  hifi
//
//  Created by Stephen Birarda on 11/26/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__HifiSockAddr__
#define __hifi__HifiSockAddr__

#include <QtNetwork/QHostAddress>

class HifiSockAddr {
public:
    HifiSockAddr();
    HifiSockAddr(const QHostAddress& address, quint16 port);
    HifiSockAddr(const HifiSockAddr& otherSockAddr);
    HifiSockAddr(const QString& hostname, quint16 hostOrderPort);
    
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
    
    friend QDebug operator<<(QDebug debug, const HifiSockAddr &hifiSockAddr);
private:
    QHostAddress _address;
    quint16 _port;
};

quint32 getHostOrderLocalAddress();

Q_DECLARE_METATYPE(HifiSockAddr)

#endif /* defined(__hifi__HifiSockAddr__) */
