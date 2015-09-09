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
#include <QtCore/QDataStream>

#include "Assignment.h"
#include "HifiSockAddr.h"
#include "NodeList.h"
#include "udt/Packet.h"
#include "udt/PacketHeaders.h"
#include "NLPacket.h"
#include "SharedUtil.h"
#include "UserActivityLogger.h"
#include "NetworkLogging.h"

#include "DomainHandler.h"

DomainHandler::DomainHandler(QObject* parent) :
    QObject(parent),
    _uuid(),
    _sockAddr(HifiSockAddr(QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
    _assignmentUUID(),
    _connectionToken(),
    _iceDomainID(),
    _iceClientID(),
    _iceServerSockAddr(),
    _icePeer(this),
    _isConnected(false),
    _settingsObject(),
    _failedSettingsRequests(0)
{
    _sockAddr.setObjectName("DomainServer");

    // if we get a socket that make sure our NetworkPeer ping timer stops
    connect(this, &DomainHandler::completedSocketDiscovery, &_icePeer, &NetworkPeer::stopPingTimer);
}

void DomainHandler::clearConnectionInfo() {
    _uuid = QUuid();
    _connectionToken = QUuid();
    
    _icePeer.reset();

    if (requiresICE()) {
        // if we connected to this domain with ICE, re-set the socket so we reconnect through the ice-server
        _sockAddr.clear();
    }

    setIsConnected(false);
}

void DomainHandler::clearSettings() {
    _settingsObject = QJsonObject();
    _failedSettingsRequests = 0;
}

void DomainHandler::softReset() {
    qCDebug(networking) << "Resetting current domain connection information.";
    clearConnectionInfo();
    clearSettings();
}

void DomainHandler::hardReset() {
    softReset();

    qCDebug(networking) << "Hard reset in NodeList DomainHandler.";
    _iceDomainID = QUuid();
    _iceServerSockAddr = HifiSockAddr();
    _hostname = QString();
    _sockAddr.clear();

    // clear any pending path we may have wanted to ask the previous DS about
    _pendingPath.clear();
}

void DomainHandler::setSockAddr(const HifiSockAddr& sockAddr, const QString& hostname) {
    if (_sockAddr != sockAddr) {
        // we should reset on a sockAddr change
        hardReset();
        // change the sockAddr
        _sockAddr = sockAddr;
    }

    if (!_sockAddr.isNull()) {
        DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    }

    // some callers may pass a hostname, this is not to be used for lookup but for DTLS certificate verification
    _hostname = hostname;
}

void DomainHandler::setUUID(const QUuid& uuid) {
    if (uuid != _uuid) {
        _uuid = uuid;
        qCDebug(networking) << "Domain ID changed to" << uuidStringWithoutCurlyBraces(_uuid);
    }
}

void DomainHandler::setHostnameAndPort(const QString& hostname, quint16 port) {

    if (hostname != _hostname || _sockAddr.getPort() != port) {
        // re-set the domain info so that auth information is reloaded
        hardReset();

        if (hostname != _hostname) {
            // set the new hostname
            _hostname = hostname;

            qCDebug(networking) << "Updated domain hostname to" << _hostname;

            // re-set the sock addr to null and fire off a lookup of the IP address for this domain-server's hostname
            qCDebug(networking, "Looking up DS hostname %s.", _hostname.toLocal8Bit().constData());
            QHostInfo::lookupHost(_hostname, this, SLOT(completedHostnameLookup(const QHostInfo&)));

            DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainHostname);

            UserActivityLogger::getInstance().changedDomain(_hostname);
            emit hostnameChanged(_hostname);
        }

        if (_sockAddr.getPort() != port) {
            qCDebug(networking) << "Updated domain port to" << port;
        }

        // grab the port by reading the string after the colon
        _sockAddr.setPort(port);
    }
}

void DomainHandler::setIceServerHostnameAndID(const QString& iceServerHostname, const QUuid& id) {
    if (id != _uuid) {
        // re-set the domain info to connect to new domain
        hardReset();
        
        // refresh our ICE client UUID to something new
        _iceClientID = QUuid::createUuid();
        
        _iceDomainID = id;

        HifiSockAddr* replaceableSockAddr = &_iceServerSockAddr;
        replaceableSockAddr->~HifiSockAddr();
        replaceableSockAddr = new (replaceableSockAddr) HifiSockAddr(iceServerHostname, ICE_SERVER_DEFAULT_PORT);
        _iceServerSockAddr.setObjectName("IceServer");

        auto nodeList = DependencyManager::get<NodeList>();

        nodeList->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetICEServerHostname);

        if (_iceServerSockAddr.getAddress().isNull()) {
            // connect to lookup completed for ice-server socket so we can request a heartbeat once hostname is looked up
            connect(&_iceServerSockAddr, &HifiSockAddr::lookupCompleted, this, &DomainHandler::completedIceServerHostnameLookup);
        } else {
            completedIceServerHostnameLookup();
        }

        qCDebug(networking) << "ICE required to connect to domain via ice server at" << iceServerHostname;
    }
}

void DomainHandler::activateICELocalSocket() {
    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    _sockAddr = _icePeer.getLocalSocket();
    _hostname = _sockAddr.getAddress().toString();
    emit completedSocketDiscovery();
}

void DomainHandler::activateICEPublicSocket() {
    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);
    _sockAddr = _icePeer.getPublicSocket();
    _hostname = _sockAddr.getAddress().toString();
    emit completedSocketDiscovery();
}

void DomainHandler::completedHostnameLookup(const QHostInfo& hostInfo) {
    for (int i = 0; i < hostInfo.addresses().size(); i++) {
        if (hostInfo.addresses()[i].protocol() == QAbstractSocket::IPv4Protocol) {
            _sockAddr.setAddress(hostInfo.addresses()[i]);

            DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetDomainSocket);

            qCDebug(networking, "DS at %s is at %s", _hostname.toLocal8Bit().constData(),
                   _sockAddr.getAddress().toString().toLocal8Bit().constData());

            emit completedSocketDiscovery();

            return;
        }
    }

    // if we got here then we failed to lookup the address
    qCDebug(networking, "Failed domain server lookup");
}

void DomainHandler::completedIceServerHostnameLookup() {
    qDebug() << "ICE server socket is at" << _iceServerSockAddr;

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetICEServerSocket);

    // emit our signal so we can send a heartbeat to ice-server immediately
    emit iceSocketAndIDReceived();
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
    // TODO: the nodes basically lock if they don't get a response - add a timeout to this so that they at least restart
    // if they can't get settings
    
    NodeType_t owningNodeType = DependencyManager::get<NodeList>()->getOwnerType();
    if (owningNodeType == NodeType::Agent) {
        // for now the agent nodes don't need any settings - this allows local assignment-clients
        // to connect to a domain that is using automatic networking (since we don't have TCP hole punch yet)
        _settingsObject = QJsonObject();
        emit settingsReceived(_settingsObject);
    } else {
        if (_settingsObject.isEmpty()) {
            qCDebug(networking) << "Requesting settings from domain server";

            Assignment::Type assignmentType = Assignment::typeForNodeType(DependencyManager::get<NodeList>()->getOwnerType());

            auto packet = NLPacket::create(PacketType::DomainSettingsRequest, sizeof(assignmentType), true, false);
            packet->writePrimitive(assignmentType);

            auto nodeList = DependencyManager::get<LimitedNodeList>();
            nodeList->sendPacket(std::move(packet), _sockAddr);
        }
    }
}

void DomainHandler::processSettingsPacketList(QSharedPointer<NLPacketList> packetList) {
    auto data = packetList->getMessage();

    _settingsObject = QJsonDocument::fromJson(data).object();

    qCDebug(networking) << "Received domain settings: \n" << QString(data);

    // reset failed settings requests to 0, we got them
    _failedSettingsRequests = 0;

    emit settingsReceived(_settingsObject);
}

void DomainHandler::processICEPingReplyPacket(QSharedPointer<NLPacket> packet) {
    const HifiSockAddr& senderSockAddr = packet->getSenderSockAddr();
    qCDebug(networking) << "Received reply from domain-server on" << senderSockAddr;

    if (getIP().isNull()) {
        // for now we're unsafely assuming this came back from the domain
        if (senderSockAddr == _icePeer.getLocalSocket()) {
            qCDebug(networking) << "Connecting to domain using local socket";
            activateICELocalSocket();
        } else if (senderSockAddr == _icePeer.getPublicSocket()) {
            qCDebug(networking) << "Conecting to domain using public socket";
            activateICEPublicSocket();
        } else {
            qCDebug(networking) << "Reply does not match either local or public socket for domain. Will not connect.";
        }
    }
}

void DomainHandler::processDTLSRequirementPacket(QSharedPointer<NLPacket> dtlsRequirementPacket) {
    // figure out the port that the DS wants us to use for us to talk to them with DTLS
    unsigned short dtlsPort;
    dtlsRequirementPacket->readPrimitive(&dtlsPort);

    qCDebug(networking) << "domain-server DTLS port changed to" << dtlsPort << "- Enabling DTLS.";

    _sockAddr.setPort(dtlsPort);

//    initializeDTLSSession();
}

void DomainHandler::processICEResponsePacket(QSharedPointer<NLPacket> icePacket) {
    if (_icePeer.hasSockets()) {
        qDebug() << "Received an ICE peer packet for domain-server but we already have sockets. Not processing.";
        // bail on processing this packet if our ice peer doesn't have sockets
        return;
    }

    QDataStream iceResponseStream(icePacket.data());

    iceResponseStream >> _icePeer;

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveDSPeerInformation);

    if (_icePeer.getUUID() != _iceDomainID) {
        qCDebug(networking) << "Received a network peer with ID that does not match current domain. Will not attempt connection.";
        _icePeer.reset();
    } else {
        qCDebug(networking) << "Received network peer object for domain -" << _icePeer;

        // ask the peer object to start its ping timer
        _icePeer.startPingTimer();

        // emit our signal so the NodeList knows to send a ping immediately
        emit icePeerSocketsReceived();
    }
}
