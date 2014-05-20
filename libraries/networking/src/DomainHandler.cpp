//
//  DomainHandler.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NodeList.h"
#include "PacketHeaders.h"

#include "DomainHandler.h"

DomainHandler::DomainHandler(QObject* parent) :
    QObject(parent),
    _uuid(),
    _sockAddr(HifiSockAddr(QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
    _assignmentUUID(),
    _isConnected(false),
    _handshakeTimer(NULL)
{
    
}

void DomainHandler::clearConnectionInfo() {
    _uuid = QUuid();
    _isConnected = false;
    
    if (_handshakeTimer) {
        _handshakeTimer->stop();
        delete _handshakeTimer;
        _handshakeTimer = NULL;
    }
}

void DomainHandler::reset() {
    clearConnectionInfo();
    _hostname = QString();
    _sockAddr.setAddress(QHostAddress::Null);
}

void DomainHandler::setSockAddr(const HifiSockAddr& sockAddr, const QString& hostname) {
    if (_sockAddr != sockAddr) {
        // we should reset on a sockAddr change
        reset();
        // change the sockAddr
        _sockAddr = sockAddr;
    }
    
    // some callers may pass a hostname, this is not to be used for lookup but for DTLS certificate verification
    _hostname = hostname;
}

void DomainHandler::setHostname(const QString& hostname) {
    
    if (hostname != _hostname) {
        // re-set the domain info so that auth information is reloaded
        reset();
        
        int colonIndex = hostname.indexOf(':');
        
        if (colonIndex > 0) {
            // the user has included a custom DS port with the hostname
            
            // the new hostname is everything up to the colon
            _hostname = hostname.left(colonIndex);
            
            // grab the port by reading the string after the colon
            _sockAddr.setPort(atoi(hostname.mid(colonIndex + 1, hostname.size()).toLocal8Bit().constData()));
            
            qDebug() << "Updated hostname to" << _hostname << "and port to" << _sockAddr.getPort();
            
        } else {
            // no port included with the hostname, simply set the member variable and reset the domain server port to default
            _hostname = hostname;
            _sockAddr.setPort(DEFAULT_DOMAIN_SERVER_PORT);
        }
        
        // re-set the sock addr to null and fire off a lookup of the IP address for this domain-server's hostname
        qDebug("Looking up DS hostname %s.", _hostname.toLocal8Bit().constData());
        QHostInfo::lookupHost(_hostname, this, SLOT(completedHostnameLookup(const QHostInfo&)));
        
        emit hostnameChanged(_hostname);
    }
}

void DomainHandler::completedHostnameLookup(const QHostInfo& hostInfo) {
    for (int i = 0; i < hostInfo.addresses().size(); i++) {
        if (hostInfo.addresses()[i].protocol() == QAbstractSocket::IPv4Protocol) {
            _sockAddr.setAddress(hostInfo.addresses()[i]);
            qDebug("DS at %s is at %s", _hostname.toLocal8Bit().constData(),
                   _sockAddr.getAddress().toString().toLocal8Bit().constData());
            return;
        }        
    }
    
    // if we got here then we failed to lookup the address
    qDebug("Failed domain server lookup");
}

void DomainHandler::setIsConnected(bool isConnected) {
    if (_isConnected != isConnected) {
        _isConnected = isConnected;
        
        if (_isConnected) {
            emit connectedToDomain(_hostname);
        }
    }
}

void DomainHandler::parseDTLSRequirementPacket(const QByteArray& dtlsRequirementPacket) {
    // figure out the port that the DS wants us to use for us to talk to them with DTLS
    int numBytesPacketHeader = numBytesForPacketHeader(dtlsRequirementPacket);
    
    unsigned short dtlsPort = 0;
    memcpy(&dtlsPort, dtlsRequirementPacket.data() + numBytesPacketHeader, sizeof(dtlsPort));
    
    qDebug() << "domain-server DTLS port changed to" << dtlsPort << "- Enabling DTLS.";
    
    _sockAddr.setPort(dtlsPort);
    
//    initializeDTLSSession();
}