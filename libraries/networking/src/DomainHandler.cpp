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

#include <math.h>

#include <QtCore/QJsonDocument>

#include "Assignment.h"
#include "NodeList.h"
#include "PacketHeaders.h"
#include "UserActivityLogger.h"

#include "DomainHandler.h"

DomainHandler::DomainHandler(QObject* parent) :
    QObject(parent),
    _uuid(),
    _sockAddr(HifiSockAddr(QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
    _assignmentUUID(),
    _iceDomainID(),
    _iceClientID(),
    _iceServerSockAddr(),
    _icePeer(),
    _isConnected(false),
    _handshakeTimer(NULL),
    _settingsObject(),
    _failedSettingsRequests(0)
{
    
}

void DomainHandler::clearConnectionInfo() {
    _uuid = QUuid();
    
    _icePeer = NetworkPeer();
    
    if (requiresICE()) {
        // if we connected to this domain with ICE, re-set the socket so we reconnect through the ice-server
        _sockAddr.setAddress(QHostAddress::Null);
    }
    
    _isConnected = false;
    
    emit disconnectedFromDomain();
    
    if (_handshakeTimer) {
        _handshakeTimer->stop();
        delete _handshakeTimer;
        _handshakeTimer = NULL;
    }
}

void DomainHandler::clearSettings() {
    _settingsObject = QJsonObject();
    _failedSettingsRequests = 0;
}

void DomainHandler::softReset() {
    clearConnectionInfo();
    clearSettings();
}

void DomainHandler::hardReset() {
    softReset();
    _iceDomainID = QUuid();
    _hostname = QString();
    _sockAddr.setAddress(QHostAddress::Null);
}

void DomainHandler::setSockAddr(const HifiSockAddr& sockAddr, const QString& hostname) {
    if (_sockAddr != sockAddr) {
        // we should reset on a sockAddr change
        hardReset();
        // change the sockAddr
        _sockAddr = sockAddr;
    }
    
    // some callers may pass a hostname, this is not to be used for lookup but for DTLS certificate verification
    _hostname = hostname;
}

void DomainHandler::setUUID(const QUuid& uuid) {
    if (uuid != _uuid) {
        _uuid = uuid;
        qDebug() << "Domain uuid changed to" << uuidStringWithoutCurlyBraces(_uuid);
    }
}

QString DomainHandler::hostnameWithoutPort(const QString& hostname) {
    int colonIndex = hostname.indexOf(':');
    return colonIndex > 0 ? hostname.left(colonIndex) : hostname;
}

bool DomainHandler::isCurrentHostname(const QString& hostname) {
    return hostnameWithoutPort(hostname) == _hostname;
}

void DomainHandler::setHostname(const QString& hostname) {
    
    if (hostname != _hostname) {
        // re-set the domain info so that auth information is reloaded
        hardReset();
        
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
        
        UserActivityLogger::getInstance().changedDomain(_hostname);
        emit hostnameChanged(_hostname);
    }
}

void DomainHandler::setIceServerHostnameAndID(const QString& iceServerHostname, const QUuid& id) {
    if (id != _uuid) {
        // re-set the domain info to connect to new domain
        hardReset();
        
        _iceDomainID = id;
        _iceServerSockAddr = HifiSockAddr(iceServerHostname, ICE_SERVER_DEFAULT_PORT);
        
        // refresh our ICE client UUID to something new
        _iceClientID = QUuid::createUuid();
        
        qDebug() << "ICE required to connect to domain via ice server at" << iceServerHostname;
    }
}

void DomainHandler::activateICELocalSocket() {
    _sockAddr = _icePeer.getLocalSocket();
    _hostname = _sockAddr.getAddress().toString();
}

void DomainHandler::activateICEPublicSocket() {
    _sockAddr = _icePeer.getPublicSocket();
    _hostname = _sockAddr.getAddress().toString();
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
            
            // we've connected to new domain - time to ask it for global settings
            requestDomainSettings();
        } else {
            emit disconnectedFromDomain();
        }
    }
}

void DomainHandler::requestDomainSettings() {
    NodeType_t owningNodeType = NodeList::getInstance()->getOwnerType();
    if (owningNodeType == NodeType::Agent) {
        // for now the agent nodes don't need any settings - this allows local assignment-clients
        // to connect to a domain that is using automatic networking (since we don't have TCP hole punch yet)
        _settingsObject = QJsonObject();
        emit settingsReceived(_settingsObject);
    } else {
        if (_settingsObject.isEmpty()) {
            // setup the URL required to grab settings JSON
            QUrl settingsJSONURL;
            settingsJSONURL.setScheme("http");
            settingsJSONURL.setHost(_hostname);
            settingsJSONURL.setPort(DOMAIN_SERVER_HTTP_PORT);
            settingsJSONURL.setPath("/settings.json");
            Assignment::Type assignmentType = Assignment::typeForNodeType(NodeList::getInstance()->getOwnerType());
            settingsJSONURL.setQuery(QString("type=%1").arg(assignmentType));
            
            qDebug() << "Requesting domain-server settings at" << settingsJSONURL.toString();
            
            QNetworkReply* reply = NetworkAccessManager::getInstance().get(QNetworkRequest(settingsJSONURL));
            connect(reply, &QNetworkReply::finished, this, &DomainHandler::settingsRequestFinished);
        }
    }
}

const int MAX_SETTINGS_REQUEST_FAILED_ATTEMPTS = 5;

void DomainHandler::settingsRequestFinished() {
    QNetworkReply* settingsReply = reinterpret_cast<QNetworkReply*>(sender());
    
    int replyCode = settingsReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    
    if (settingsReply->error() == QNetworkReply::NoError && replyCode != 301 && replyCode != 302) {
        // parse the JSON to a QJsonObject and save it
        _settingsObject = QJsonDocument::fromJson(settingsReply->readAll()).object();
        
        qDebug() << "Received domain settings.";
        emit settingsReceived(_settingsObject);
        
        // reset failed settings requests to 0, we got them
        _failedSettingsRequests = 0;
    } else {
        // error grabbing the settings - in some cases this means we are stuck
        // so we should retry until we get it
        qDebug() << "Error getting domain settings -" << settingsReply->errorString() << "- retrying";
        
        if (++_failedSettingsRequests >= MAX_SETTINGS_REQUEST_FAILED_ATTEMPTS) {
            qDebug() << "Failed to retreive domain-server settings" << MAX_SETTINGS_REQUEST_FAILED_ATTEMPTS
                << "times. Re-setting connection to domain.";
            clearSettings();
            clearConnectionInfo();
            emit settingsReceiveFail();
        } else {
            requestDomainSettings();
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

void DomainHandler::processICEResponsePacket(const QByteArray& icePacket) {
    QDataStream iceResponseStream(icePacket);
    iceResponseStream.skipRawData(numBytesForPacketHeader(icePacket));
    
    NetworkPeer packetPeer;
    iceResponseStream >> packetPeer;
    
    if (packetPeer.getUUID() != _iceDomainID) {
        qDebug() << "Received a network peer with ID that does not match current domain. Will not attempt connection.";
    } else {
        qDebug() << "Received network peer object for domain -" << packetPeer;
        _icePeer = packetPeer;
        
        emit requestICEConnectionAttempt();
    }
}
