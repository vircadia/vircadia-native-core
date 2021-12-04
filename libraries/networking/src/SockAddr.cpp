//
//  SockAddr.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 11/26/2013.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SockAddr.h"

#include <qdatastream.h>
#include <qhostinfo.h>
#include <qnetworkinterface.h>

#include "NetworkLogging.h"

#ifdef WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#include <netinet/in.h>
#endif

int sockAddrMetaTypeId = qRegisterMetaType<SockAddr>();

SockAddr::SockAddr() :
    _socketType(SocketType::Unknown),
    _address(),
    _port(0)
{
}

SockAddr::SockAddr(SocketType socketType, const QHostAddress& address, quint16 port) :
    _socketType(socketType),
    _address(address),
    _port(port)
{
}

SockAddr::SockAddr(const SockAddr& otherSockAddr) :
    QObject(),
    _socketType(otherSockAddr._socketType),
    _address(otherSockAddr._address),
    _port(otherSockAddr._port)
{
    setObjectName(otherSockAddr.objectName());
}

SockAddr& SockAddr::operator=(const SockAddr& rhsSockAddr) {
    setObjectName(rhsSockAddr.objectName());
    _socketType = rhsSockAddr._socketType;
    _address = rhsSockAddr._address;
    _port = rhsSockAddr._port;
    return *this;
}

SockAddr::SockAddr(SocketType socketType, const QString& hostname, quint16 hostOrderPort, bool shouldBlockForLookup) :
    _socketType(socketType),
    _address(hostname),
    _port(hostOrderPort)
{
    // if we parsed an IPv4 address out of the hostname, don't look it up
    if (_address.protocol() != QAbstractSocket::IPv4Protocol) {
        // lookup the IP by the hostname
        if (shouldBlockForLookup) {
            qCDebug(networking) << "Synchronously looking up IP address for hostname" << hostname;
            QHostInfo result = QHostInfo::fromName(hostname);
            handleLookupResult(result);
        } else {
            int lookupID = QHostInfo::lookupHost(hostname, this, SLOT(handleLookupResult(QHostInfo)));
            qCDebug(networking) << "Asynchronously looking up IP address for hostname" << hostname << "- lookup ID is" << lookupID;
        }
    }
}

void SockAddr::swap(SockAddr& otherSockAddr) {
    using std::swap;

    swap(_socketType, otherSockAddr._socketType);
    swap(_address, otherSockAddr._address);
    swap(_port, otherSockAddr._port);
    
    // Swap objects name
    auto temp = otherSockAddr.objectName();
    otherSockAddr.setObjectName(objectName());
    setObjectName(temp);
}

bool SockAddr::operator==(const SockAddr& rhsSockAddr) const {
    return _socketType == rhsSockAddr._socketType && _address == rhsSockAddr._address && _port == rhsSockAddr._port;
}

void SockAddr::handleLookupResult(const QHostInfo& hostInfo) {
    if (hostInfo.error() != QHostInfo::NoError) {
        qCDebug(networking) << "Lookup failed for" << hostInfo.lookupId() << ":" << hostInfo.errorString();
        emit lookupFailed();
    } else {
        foreach(const QHostAddress& address, hostInfo.addresses()) {
            // just take the first IPv4 address
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                _address = address;
                qCDebug(networking) << "QHostInfo lookup result for"
                    << hostInfo.hostName() << "with lookup ID" << hostInfo.lookupId() << "is" << address.toString();
                emit lookupCompleted();
                break;
            }
        }
    }
}

QString SockAddr::toString() const {
    return socketTypeToString(_socketType) + " " + _address.toString() + ":" + QString::number(_port);
}

QString SockAddr::toShortString() const {
    return _address.toString() + ":" + QString::number(_port);
}

bool SockAddr::hasPrivateAddress() const {
    // an address is private if it is loopback or falls in any of the RFC1918 address spaces
    const QPair<QHostAddress, int> TWENTY_FOUR_BIT_BLOCK = { QHostAddress("10.0.0.0"), 8 };
    const QPair<QHostAddress, int> TWENTY_BIT_BLOCK = { QHostAddress("172.16.0.0") , 12 };
    const QPair<QHostAddress, int> SIXTEEN_BIT_BLOCK = { QHostAddress("192.168.0.0"), 16 };

    return _address.isLoopback()
        || _address.isInSubnet(TWENTY_FOUR_BIT_BLOCK)
        || _address.isInSubnet(TWENTY_BIT_BLOCK)
        || _address.isInSubnet(SIXTEEN_BIT_BLOCK);
}

QDebug operator<<(QDebug debug, const SockAddr& sockAddr) {
    debug.nospace() 
        << (sockAddr._socketType != SocketType::Unknown 
            ? (socketTypeToString(sockAddr._socketType) + " ").toLocal8Bit().constData() : "")
        << sockAddr._address.toString().toLocal8Bit().constData() << ":" << sockAddr._port;
    return debug.space();
}

QDataStream& operator<<(QDataStream& dataStream, const SockAddr& sockAddr) {
    // Don't include socket type because ICE packets must not have it.
    dataStream << sockAddr._address << sockAddr._port;
    return dataStream;
}

QDataStream& operator>>(QDataStream& dataStream, SockAddr& sockAddr) {
    // Don't include socket type because ICE packets must not have it.
    dataStream >> sockAddr._address >> sockAddr._port;
    return dataStream;
}

uint qHash(const SockAddr& key, uint seed) {
    // use the existing QHostAddress and quint16 hash functions to get our hash
    return qHash(key.getAddress(), seed) ^ qHash(key.getPort(), seed);
}
