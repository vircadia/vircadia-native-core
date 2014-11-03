//
//  HifiSockAddr.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 11/26/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>

#include "HifiSockAddr.h"

static int hifiSockAddrMetaTypeId = qMetaTypeId<HifiSockAddr>();

HifiSockAddr::HifiSockAddr() :
    _address(),
    _port(0)
{
    
}

HifiSockAddr::HifiSockAddr(const QHostAddress& address, quint16 port) :
    _address(address),
    _port(port)
{
    
}

HifiSockAddr::HifiSockAddr(const HifiSockAddr& otherSockAddr) {
    _address = otherSockAddr._address;
    _port = otherSockAddr._port;
}

HifiSockAddr::HifiSockAddr(const QString& hostname, quint16 hostOrderPort) {
    // lookup the IP by the hostname
    QHostInfo hostInfo = QHostInfo::fromName(hostname);
    foreach(const QHostAddress& address, hostInfo.addresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            _address = address;
            _port = hostOrderPort;
        }
    }
}

HifiSockAddr::HifiSockAddr(const sockaddr* sockaddr) {
    _address = QHostAddress(sockaddr);
    
    if (sockaddr->sa_family == AF_INET) {
        _port = ntohs(reinterpret_cast<const sockaddr_in*>(sockaddr)->sin_port);
    } else {
        _port = ntohs(reinterpret_cast<const sockaddr_in6*>(sockaddr)->sin6_port);
    }
}

HifiSockAddr& HifiSockAddr::operator=(const HifiSockAddr& rhsSockAddr) {
    _address = rhsSockAddr._address;
    _port = rhsSockAddr._port;
    
    return *this;
}

void HifiSockAddr::swap(HifiSockAddr& otherSockAddr) {
    using std::swap;
    
    swap(_address, otherSockAddr._address);
    swap(_port, otherSockAddr._port);
}

bool HifiSockAddr::operator==(const HifiSockAddr& rhsSockAddr) const {
    return _address == rhsSockAddr._address && _port == rhsSockAddr._port;
}

QDebug operator<<(QDebug debug, const HifiSockAddr& sockAddr) {
    debug.nospace() << sockAddr._address.toString().toLocal8Bit().constData() << ":" << sockAddr._port;
    return debug.space();
}

QDataStream& operator<<(QDataStream& dataStream, const HifiSockAddr& sockAddr) {
    dataStream << sockAddr._address << sockAddr._port;
    return dataStream;
}

QDataStream& operator>>(QDataStream& dataStream, HifiSockAddr& sockAddr) {
    dataStream >> sockAddr._address >> sockAddr._port;
    return dataStream;
}

QHostAddress getLocalAddress() {
    
    QHostAddress localAddress;
    
    foreach(const QNetworkInterface &networkInterface, QNetworkInterface::allInterfaces()) {
        if (networkInterface.flags() & QNetworkInterface::IsUp
            && networkInterface.flags() & QNetworkInterface::IsRunning
            && networkInterface.flags() & ~QNetworkInterface::IsLoopBack) {
            // we've decided that this is the active NIC
            // enumerate it's addresses to grab the IPv4 address
            foreach(const QNetworkAddressEntry &entry, networkInterface.addressEntries()) {
                // make sure it's an IPv4 address that isn't the loopback
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                    
                    // set our localAddress and break out
                    localAddress = entry.ip();
                    break;
                }
            }
        }
        
        if (!localAddress.isNull()) {
            break;
        }
    }
    
    // return the looked up local address
    return localAddress;
}

uint qHash(const HifiSockAddr& key, uint seed) {
    // use the existing QHostAddress and quint16 hash functions to get our hash
    return qHash(key.getAddress(), seed) + qHash(key.getPort(), seed);
}
