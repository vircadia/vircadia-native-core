//
//  SockAddr.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 11/26/2013.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SockAddr_h
#define hifi_SockAddr_h

#include <cstdint>
#include <string>

struct sockaddr;

#include <QtNetwork/QHostInfo>

#include "SocketType.h"


class SockAddr : public QObject {
    Q_OBJECT
public:
    SockAddr();
    SockAddr(SocketType socketType, const QHostAddress& address, quint16 port);
    SockAddr(const SockAddr& otherSockAddr);
    SockAddr(SocketType socketType, const QString& hostname, quint16 hostOrderPort, bool shouldBlockForLookup = false);

    bool isNull() const { return _address.isNull() && _port == 0; }
    void clear() { _address.clear(); _port = 0;}

    SockAddr& operator=(const SockAddr& rhsSockAddr);
    void swap(SockAddr& otherSockAddr);

    bool operator==(const SockAddr& rhsSockAddr) const;
    bool operator!=(const SockAddr& rhsSockAddr) const { return !(*this == rhsSockAddr); }

    SocketType getType() const { return _socketType; }
    SocketType* getSocketTypePointer() { return &_socketType; }
    void setType(const SocketType socketType) { _socketType = socketType; }

    const QHostAddress& getAddress() const { return _address; }
    QHostAddress* getAddressPointer() { return &_address; }
    void setAddress(const QHostAddress& address) { _address = address; }

    quint16 getPort() const { return _port; }
    quint16* getPortPointer() { return &_port; }
    void setPort(quint16 port) { _port = port; }

    static int packSockAddr(unsigned char* packetData, const SockAddr& packSockAddr);
    static int unpackSockAddr(const unsigned char* packetData, SockAddr& unpackDestSockAddr);

    QString toString() const;
    QString toShortString() const;

    bool hasPrivateAddress() const; // checks if the address behind this sock addr is private per RFC 1918

    friend QDebug operator<<(QDebug debug, const SockAddr& sockAddr);
    friend QDataStream& operator<<(QDataStream& dataStream, const SockAddr& sockAddr);
    friend QDataStream& operator>>(QDataStream& dataStream, SockAddr& sockAddr);

private slots:
    void handleLookupResult(const QHostInfo& hostInfo);
signals:
    void lookupCompleted();
    void lookupFailed();
private:
    SocketType _socketType { SocketType::Unknown };
    QHostAddress _address;
    quint16 _port;
};

uint qHash(const SockAddr& key, uint seed);

namespace std {
    template <>
    struct hash<SockAddr> {
        // NOTE: this hashing specifically ignores IPv6 addresses - if we begin to support those we will need
        // to conditionally hash the bytes that represent an IPv6 address
        size_t operator()(const SockAddr& sockAddr) const {
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

Q_DECLARE_METATYPE(SockAddr);

#endif // hifi_SockAddr_h
