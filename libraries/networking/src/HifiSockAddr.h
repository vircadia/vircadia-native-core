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

#include <cstdint>
#include <string>

#ifdef WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <netinet/in.h>
#endif

#include <QtNetwork/QHostInfo>

class HifiSockAddr : public QObject {
    Q_OBJECT
public:
    HifiSockAddr();
    HifiSockAddr(const QHostAddress& address, quint16 port);
    HifiSockAddr(const HifiSockAddr& otherSockAddr);
    HifiSockAddr(const QString& hostname, quint16 hostOrderPort, bool shouldBlockForLookup = false);
    HifiSockAddr(const sockaddr* sockaddr);

    bool isNull() const { return _address.isNull() && _port == 0; }
    void clear() { _address = QHostAddress::Null; _port = 0;}

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
signals:
    void lookupCompleted();
    void lookupFailed();
private:
    QHostAddress _address;
    quint16 _port;
};

uint qHash(const HifiSockAddr& key, uint seed);

namespace std {
    template <>
    struct hash<HifiSockAddr> {
        // NOTE: this hashing specifically ignores IPv6 addresses - if we begin to support those we will need
        // to conditionally hash the bytes that represent an IPv6 address
        size_t operator()(const HifiSockAddr& sockAddr) const {
            // use XOR of implemented std::hash templates for new hash
            // depending on the type of address we're looking at
            
            if (sockAddr.getAddress().protocol() == QAbstractSocket::IPv4Protocol) {
                return hash<uint32_t>()((uint32_t) sockAddr.getAddress().toIPv4Address())
                    ^ hash<uint16_t>()((uint16_t) sockAddr.getPort());
            } else {
                // NOTE: if we start to use IPv6 addresses, it's possible their hashing
                // can be faster by XORing the hash for each 64 bits in the address
                return hash<string>()(sockAddr.getAddress().toString().toStdString())
                    ^ hash<uint16_t>()((uint16_t) sockAddr.getPort());
            }
        }
    };
}

Q_DECLARE_METATYPE(HifiSockAddr);

#endif // hifi_HifiSockAddr_h
