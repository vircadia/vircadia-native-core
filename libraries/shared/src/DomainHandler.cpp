//
//  DomainHandler.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <gnutls/dtls.h>

#include "NodeList.h"
#include "PacketHeaders.h"

#include "DomainHandler.h"

DomainHandler::DomainHandler(QObject* parent) :
    QObject(parent),
    _uuid(),
    _sockAddr(HifiSockAddr(QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
    _assignmentUUID(),
    _isConnected(false),
    _dtlsSession(NULL),
    _handshakeTimer(NULL)
{
    
}

DomainHandler::~DomainHandler() {
    delete _dtlsSession;
}

void DomainHandler::clearConnectionInfo() {
    _uuid = QUuid();
    _isConnected = false;
    
    delete _dtlsSession;
    _dtlsSession = NULL;
    
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
    
    delete _dtlsSession;
    _dtlsSession = NULL;
}

const unsigned int DTLS_HANDSHAKE_INTERVAL_MSECS = 100;

void DomainHandler::initializeDTLSSession() {
    if (!_dtlsSession) {
        _dtlsSession = new DTLSClientSession(NodeList::getInstance()->getDTLSSocket(), _sockAddr);
        
        // start a timer to complete the handshake process
        _handshakeTimer = new QTimer(this);
        connect(_handshakeTimer, &QTimer::timeout, this, &DomainHandler::completeDTLSHandshake);
        
        // start the handshake right now
        completeDTLSHandshake();
        
        // start the timer to finish off the handshake
        _handshakeTimer->start(DTLS_HANDSHAKE_INTERVAL_MSECS);
    }
}

void DomainHandler::setSockAddr(const HifiSockAddr& sockAddr) {
    if (_sockAddr != sockAddr) {
        // we should reset on a sockAddr change
        reset();
        // change the sockAddr
        _sockAddr = sockAddr;
    }
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

void DomainHandler::completeDTLSHandshake() {
    int handshakeReturn = gnutls_handshake(*_dtlsSession->getGnuTLSSession());
    
    if (handshakeReturn == 0) {
        // we've shaken hands, so we're good to go now
        _dtlsSession->setCompletedHandshake(true);
        
        _handshakeTimer->stop();
        delete _handshakeTimer;
        _handshakeTimer = NULL;
        
        // emit a signal so NodeList can handle incoming DTLS packets
        emit completedDTLSHandshake();
        
    } else if (gnutls_error_is_fatal(handshakeReturn)) {
        // this was a fatal error handshaking, so remove this session
        qDebug() << "Fatal error -" << gnutls_strerror(handshakeReturn)
            << "- during DTLS handshake with DS at" << getHostname();
        
        clearConnectionInfo();
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
    
    initializeDTLSSession();
}