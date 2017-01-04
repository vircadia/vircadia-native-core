//
//  NodeList.cpp
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NodeList.h"

#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QJsonDocument>
#include <QtCore/QMetaEnum>
#include <QtCore/QUrl>
#include <QtCore/QThread>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QNetworkInterface>

#include <LogHandler.h>
#include <UUID.h>

#include "AccountManager.h"
#include "AddressManager.h"
#include "Assignment.h"
#include "HifiSockAddr.h"
#include "FingerprintUtils.h"

#include "NetworkLogging.h"
#include "udt/PacketHeaders.h"
#include "SharedUtil.h"
#include <Trace.h>

const int KEEPALIVE_PING_INTERVAL_MS = 1000;

NodeList::NodeList(char newOwnerType, int socketListenPort, int dtlsListenPort) :
    LimitedNodeList(socketListenPort, dtlsListenPort),
    _ownerType(newOwnerType),
    _nodeTypesOfInterest(),
    _domainHandler(this),
    _numNoReplyDomainCheckIns(0),
    _assignmentServerSocket(),
    _keepAlivePingTimer(this)
{
    setCustomDeleter([](Dependency* dependency){
        static_cast<NodeList*>(dependency)->deleteLater();
    });
    
    auto addressManager = DependencyManager::get<AddressManager>();

    // handle domain change signals from AddressManager
    connect(addressManager.data(), &AddressManager::possibleDomainChangeRequired,
            &_domainHandler, &DomainHandler::setSocketAndID);

    connect(addressManager.data(), &AddressManager::possibleDomainChangeRequiredViaICEForID,
            &_domainHandler, &DomainHandler::setIceServerHostnameAndID);

    // handle a request for a path change from the AddressManager
    connect(addressManager.data(), &AddressManager::pathChangeRequired, this, &NodeList::handleDSPathQuery);

    // in case we don't know how to talk to DS when a path change is requested
    // fire off any pending DS path query when we get socket information
    connect(&_domainHandler, &DomainHandler::connectedToDomain, this, &NodeList::sendPendingDSPathQuery);

    // send a domain server check in immediately once the DS socket is known
    connect(&_domainHandler, &DomainHandler::completedSocketDiscovery, this, &NodeList::sendDomainServerCheckIn);

    // send a domain server check in immediately if there is a public socket change
    connect(this, &LimitedNodeList::publicSockAddrChanged, this, &NodeList::sendDomainServerCheckIn);

    // clear our NodeList when the domain changes
    connect(&_domainHandler, &DomainHandler::disconnectedFromDomain, this, &NodeList::reset);

    // send an ICE heartbeat as soon as we get ice server information
    connect(&_domainHandler, &DomainHandler::iceSocketAndIDReceived, this, &NodeList::handleICEConnectionToDomainServer);

    // handle ping timeout from DomainHandler to establish a connection with auto networked domain-server
    connect(&_domainHandler.getICEPeer(), &NetworkPeer::pingTimerTimeout, this, &NodeList::pingPunchForDomainServer);

    // send a ping punch immediately
    connect(&_domainHandler, &DomainHandler::icePeerSocketsReceived, this, &NodeList::pingPunchForDomainServer);

    auto accountManager = DependencyManager::get<AccountManager>();
    
    // assume that we may need to send a new DS check in anytime a new keypair is generated 
    connect(accountManager.data(), &AccountManager::newKeypair, this, &NodeList::sendDomainServerCheckIn);

    // clear out NodeList when login is finished
    connect(accountManager.data(), &AccountManager::loginComplete , this, &NodeList::reset);

    // clear our NodeList when logout is requested
    connect(accountManager.data(), &AccountManager::logoutComplete , this, &NodeList::reset);

    // anytime we get a new node we will want to attempt to punch to it
    connect(this, &LimitedNodeList::nodeAdded, this, &NodeList::startNodeHolePunch);
    connect(this, &LimitedNodeList::nodeSocketUpdated, this, &NodeList::startNodeHolePunch);

    // anytime we get a new node we may need to re-send our set of ignored node IDs to it
    connect(this, &LimitedNodeList::nodeActivated, this, &NodeList::maybeSendIgnoreSetToNode);
    
    // setup our timer to send keepalive pings (it's started and stopped on domain connect/disconnect)
    _keepAlivePingTimer.setInterval(KEEPALIVE_PING_INTERVAL_MS);
    connect(&_keepAlivePingTimer, &QTimer::timeout, this, &NodeList::sendKeepAlivePings);
    connect(&_domainHandler, SIGNAL(connectedToDomain(QString)), &_keepAlivePingTimer, SLOT(start()));
    connect(&_domainHandler, &DomainHandler::disconnectedFromDomain, &_keepAlivePingTimer, &QTimer::stop);

    // set our sockAddrBelongsToDomainOrNode method as the connection creation filter for the udt::Socket
    using std::placeholders::_1;
    _nodeSocket.setConnectionCreationFilterOperator(std::bind(&NodeList::sockAddrBelongsToDomainOrNode, this, _1));

    // we definitely want STUN to update our public socket, so call the LNL to kick that off
    startSTUNPublicSocketUpdate();

    auto& packetReceiver = getPacketReceiver();
    packetReceiver.registerListener(PacketType::DomainList, this, "processDomainServerList");
    packetReceiver.registerListener(PacketType::Ping, this, "processPingPacket");
    packetReceiver.registerListener(PacketType::PingReply, this, "processPingReplyPacket");
    packetReceiver.registerListener(PacketType::ICEPing, this, "processICEPingPacket");
    packetReceiver.registerListener(PacketType::DomainServerAddedNode, this, "processDomainServerAddedNode");
    packetReceiver.registerListener(PacketType::DomainServerConnectionToken, this, "processDomainServerConnectionTokenPacket");
    packetReceiver.registerListener(PacketType::DomainConnectionDenied, &_domainHandler, "processDomainServerConnectionDeniedPacket");
    packetReceiver.registerListener(PacketType::DomainSettings, &_domainHandler, "processSettingsPacketList");
    packetReceiver.registerListener(PacketType::ICEServerPeerInformation, &_domainHandler, "processICEResponsePacket");
    packetReceiver.registerListener(PacketType::DomainServerRequireDTLS, &_domainHandler, "processDTLSRequirementPacket");
    packetReceiver.registerListener(PacketType::ICEPingReply, &_domainHandler, "processICEPingReplyPacket");
    packetReceiver.registerListener(PacketType::DomainServerPathResponse, this, "processDomainServerPathResponse");
    packetReceiver.registerListener(PacketType::DomainServerRemovedNode, this, "processDomainServerRemovedNode");
    packetReceiver.registerListener(PacketType::UsernameFromIDReply, this, "processUsernameFromIDReply");
}

qint64 NodeList::sendStats(QJsonObject statsObject, HifiSockAddr destination) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "sendStats", Qt::QueuedConnection,
                                  Q_ARG(QJsonObject, statsObject),
                                  Q_ARG(HifiSockAddr, destination));
        return 0;
    }

    auto statsPacketList = NLPacketList::create(PacketType::NodeJsonStats, QByteArray(), true, true);

    QJsonDocument jsonDocument(statsObject);
    statsPacketList->write(jsonDocument.toBinaryData());

    sendPacketList(std::move(statsPacketList), destination);
    return 0;
}

qint64 NodeList::sendStatsToDomainServer(QJsonObject statsObject) {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "sendStatsToDomainServer", Qt::QueuedConnection,
                                  Q_ARG(QJsonObject, statsObject));
        return 0;
    }

    return sendStats(statsObject, _domainHandler.getSockAddr());
}

void NodeList::timePingReply(ReceivedMessage& message, const SharedNodePointer& sendingNode) {
    PingType_t pingType;
    
    quint64 ourOriginalTime, othersReplyTime;
    
    message.seek(0);
    
    message.readPrimitive(&pingType);
    message.readPrimitive(&ourOriginalTime);
    message.readPrimitive(&othersReplyTime);

    quint64 now = usecTimestampNow();
    qint64 pingTime = now - ourOriginalTime;
    qint64 oneWayFlightTime = pingTime / 2; // half of the ping is our one way flight

    // The other node's expected time should be our original time plus the one way flight time
    // anything other than that is clock skew
    quint64 othersExpectedReply = ourOriginalTime + oneWayFlightTime;
    qint64 clockSkew = othersReplyTime - othersExpectedReply;

    sendingNode->setPingMs(pingTime / 1000);
    sendingNode->updateClockSkewUsec(clockSkew);

    const bool wantDebug = false;

    if (wantDebug) {
        auto averageClockSkew = sendingNode->getClockSkewUsec();
        qCDebug(networking) << "PING_REPLY from node " << *sendingNode << "\n" <<
        "                     now: " << now << "\n" <<
        "                 ourTime: " << ourOriginalTime << "\n" <<
        "                pingTime: " << pingTime << "\n" <<
        "        oneWayFlightTime: " << oneWayFlightTime << "\n" <<
        "         othersReplyTime: " << othersReplyTime << "\n" <<
        "    othersExprectedReply: " << othersExpectedReply << "\n" <<
        "               clockSkew: " << clockSkew << "[" << formatUsecTime(clockSkew) << "]" << "\n" <<
        "       average clockSkew: " << averageClockSkew << "[" << formatUsecTime(averageClockSkew) << "]";
    }
}

void NodeList::processPingPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    
    // send back a reply
    auto replyPacket = constructPingReplyPacket(*message);
    const HifiSockAddr& senderSockAddr = message->getSenderSockAddr();
    sendPacket(std::move(replyPacket), *sendingNode, senderSockAddr);

    // If we don't have a symmetric socket for this node and this socket doesn't match
    // what we have for public and local then set it as the symmetric.
    // This allows a server on a reachable port to communicate with nodes on symmetric NATs
    if (sendingNode->getSymmetricSocket().isNull()) {
        if (senderSockAddr != sendingNode->getLocalSocket() && senderSockAddr != sendingNode->getPublicSocket()) {
            sendingNode->setSymmetricSocket(senderSockAddr);
        }
    }
}

void NodeList::processPingReplyPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // activate the appropriate socket for this node, if not yet updated
    activateSocketFromNodeCommunication(*message, sendingNode);

    // set the ping time for this node for stat collection
    timePingReply(*message, sendingNode);
}

void NodeList::processICEPingPacket(QSharedPointer<ReceivedMessage> message) {
    // send back a reply
    auto replyPacket = constructICEPingReplyPacket(*message, _domainHandler.getICEClientID());
    sendPacket(std::move(replyPacket), message->getSenderSockAddr());
}

void NodeList::reset() {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "reset", Qt::BlockingQueuedConnection);
        return;
    }

    LimitedNodeList::reset();

    _numNoReplyDomainCheckIns = 0;

    // lock and clear our set of ignored IDs
    _ignoredSetLock.lockForWrite();
    _ignoredNodeIDs.clear();
    _ignoredSetLock.unlock();

    // refresh the owner UUID to the NULL UUID
    setSessionUUID(QUuid());

    if (sender() != &_domainHandler) {
        // clear the domain connection information, unless they're the ones that asked us to reset
        _domainHandler.softReset();
    }

    // if we setup the DTLS socket, also disconnect from the DTLS socket readyRead() so it can handle handshaking
    if (_dtlsSocket) {
        disconnect(_dtlsSocket, 0, this, 0);
    }
}

void NodeList::addNodeTypeToInterestSet(NodeType_t nodeTypeToAdd) {
    _nodeTypesOfInterest << nodeTypeToAdd;
}

void NodeList::addSetOfNodeTypesToNodeInterestSet(const NodeSet& setOfNodeTypes) {
    _nodeTypesOfInterest.unite(setOfNodeTypes);
}

void NodeList::sendDomainServerCheckIn() {
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "sendDomainServerCheckIn", Qt::QueuedConnection);
        return;
    }

    if (_isShuttingDown) {
        qCDebug(networking) << "Refusing to send a domain-server check in while shutting down.";
        return;
    }

    if (_publicSockAddr.isNull()) {
        // we don't know our public socket and we need to send it to the domain server
        qCDebug(networking) << "Waiting for inital public socket from STUN. Will not send domain-server check in.";
    } else if (_domainHandler.getIP().isNull() && _domainHandler.requiresICE()) {
        qCDebug(networking) << "Waiting for ICE discovered domain-server socket. Will not send domain-server check in.";
        handleICEConnectionToDomainServer();
    } else if (!_domainHandler.getIP().isNull()) {

        PacketType domainPacketType = !_domainHandler.isConnected()
            ? PacketType::DomainConnectRequest : PacketType::DomainListRequest;

        if (!_domainHandler.isConnected()) {
            qCDebug(networking) << "Sending connect request to domain-server at" << _domainHandler.getHostname();

            // is this our localhost domain-server?
            // if so we need to make sure we have an up-to-date local port in case it restarted

            if (_domainHandler.getSockAddr().getAddress() == QHostAddress::LocalHost
                || _domainHandler.getHostname() == "localhost") {

                quint16 domainPort = DEFAULT_DOMAIN_SERVER_PORT;
                getLocalServerPortFromSharedMemory(DOMAIN_SERVER_LOCAL_PORT_SMEM_KEY, domainPort);
                qCDebug(networking) << "Local domain-server port read from shared memory (or default) is" << domainPort;
                _domainHandler.setPort(domainPort);
            }

        }

        // check if we're missing a keypair we need to verify ourselves with the domain-server
        auto accountManager = DependencyManager::get<AccountManager>();
        const QUuid& connectionToken = _domainHandler.getConnectionToken();

        bool requiresUsernameSignature = !_domainHandler.isConnected() && !connectionToken.isNull();

        if (requiresUsernameSignature && !accountManager->getAccountInfo().hasPrivateKey()) {
            qWarning() << "A keypair is required to present a username signature to the domain-server"
                << "but no keypair is present. Waiting for keypair generation to complete.";
            accountManager->generateNewUserKeypair();

            // don't send the check in packet - wait for the keypair first
            return;
        }

        auto domainPacket = NLPacket::create(domainPacketType);
        
        QDataStream packetStream(domainPacket.get());

        if (domainPacketType == PacketType::DomainConnectRequest) {

#if (PR_BUILD || DEV_BUILD)
            if (_shouldSendNewerVersion) {
                domainPacket->setVersion(versionForPacketType(domainPacketType) + 1);
            }
#endif

            QUuid connectUUID;

            if (!_domainHandler.getAssignmentUUID().isNull()) {
                // this is a connect request and we're an assigned node
                // so set our packetUUID as the assignment UUID
                connectUUID = _domainHandler.getAssignmentUUID();
            } else if (_domainHandler.requiresICE()) {
                // this is a connect request and we're an interface client
                // that used ice to discover the DS
                // so send our ICE client UUID with the connect request
                connectUUID = _domainHandler.getICEClientID();
            }

            // pack the connect UUID for this connect request
            packetStream << connectUUID;

            // include the protocol version signature in our connect request
            QByteArray protocolVersionSig = protocolVersionsSignature();
            packetStream.writeBytes(protocolVersionSig.constData(), protocolVersionSig.size());

            // if possible, include the MAC address for the current interface in our connect request
            QString hardwareAddress;

            for (auto networkInterface : QNetworkInterface::allInterfaces()) {
                for (auto interfaceAddress : networkInterface.addressEntries()) {
                    if (interfaceAddress.ip() == _localSockAddr.getAddress()) {
                        // this is the interface whose local IP matches what we've detected the current IP to be
                        hardwareAddress = networkInterface.hardwareAddress();

                        // stop checking interfaces and addresses
                        break;
                    }
                }

                // stop looping if this was the current interface
                if (!hardwareAddress.isEmpty()) {
                    break;
                }
            }

            packetStream << hardwareAddress;

            // now add the machine fingerprint - a null UUID if logged in, real one if not logged in
            auto accountManager = DependencyManager::get<AccountManager>();
            packetStream << (accountManager->isLoggedIn() ? QUuid() : FingerprintUtils::getMachineFingerprint());
        }

        // pack our data to send to the domain-server including
        // the hostname information (so the domain-server can see which place name we came in on)
        packetStream << _ownerType.load() << _publicSockAddr << _localSockAddr << _nodeTypesOfInterest.toList();
        packetStream << DependencyManager::get<AddressManager>()->getPlaceName();

        if (!_domainHandler.isConnected()) {
            DataServerAccountInfo& accountInfo = accountManager->getAccountInfo();
            packetStream << accountInfo.getUsername();

            // if this is a connect request, and we can present a username signature, send it along
            if (requiresUsernameSignature && accountManager->getAccountInfo().hasPrivateKey()) {
                const QByteArray& usernameSignature = accountManager->getAccountInfo().getUsernameSignature(connectionToken);
                packetStream << usernameSignature;
            }
        }

        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendDSCheckIn);

        sendPacket(std::move(domainPacket), _domainHandler.getSockAddr());

        if (_numNoReplyDomainCheckIns >= MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            // we haven't heard back from DS in MAX_SILENT_DOMAIN_SERVER_CHECK_INS
            // so emit our signal that says that
            qCDebug(networking) << "Limit of silent domain checkins reached";
            emit limitOfSilentDomainCheckInsReached();
        }

        // increment the count of un-replied check-ins
        _numNoReplyDomainCheckIns++;
    }
}

void NodeList::handleDSPathQuery(const QString& newPath) {
    if (_domainHandler.isSocketKnown()) {
        // if we have a DS socket we assume it will get this packet and send if off right away
        sendDSPathQuery(newPath);
    } else {
        // otherwise we make it pending so that it can be sent once a connection is established
        _domainHandler.setPendingPath(newPath);
    }
}

void NodeList::sendPendingDSPathQuery() {

    QString pendingPath = _domainHandler.getPendingPath();

    if (!pendingPath.isEmpty()) {
        qCDebug(networking) << "Attemping to send pending query to DS for path" << pendingPath;

        // this is a slot triggered if we just established a network link with a DS and want to send a path query
        sendDSPathQuery(_domainHandler.getPendingPath());

        // clear whatever the pending path was
        _domainHandler.clearPendingPath();
    }
}

void NodeList::sendDSPathQuery(const QString& newPath) {
    // only send a path query if we know who our DS is or is going to be
    if (_domainHandler.isSocketKnown()) {
        // construct the path query packet
        auto pathQueryPacket = NLPacket::create(PacketType::DomainServerPathQuery, -1, true);

        // get the UTF8 representation of path query
        QByteArray pathQueryUTF8 = newPath.toUtf8();

        // get the size of the UTF8 representation of the desired path
        quint16 numPathBytes = pathQueryUTF8.size();

        if (numPathBytes + ((qint16) sizeof(numPathBytes)) < pathQueryPacket->bytesAvailableForWrite()) {
            // append the size of the path to the query packet
            pathQueryPacket->writePrimitive(numPathBytes);

            // append the path itself to the query packet
            pathQueryPacket->write(pathQueryUTF8);

            qCDebug(networking) << "Sending a path query packet for path" << newPath << "to domain-server at"
                << _domainHandler.getSockAddr();

            // send off the path query
            sendPacket(std::move(pathQueryPacket), _domainHandler.getSockAddr());
        } else {
            qCDebug(networking) << "Path" << newPath << "would make PacketType::DomainServerPathQuery packet > MAX_PACKET_SIZE." <<
                "Will not send query.";
        }
    }
}

void NodeList::processDomainServerPathResponse(QSharedPointer<ReceivedMessage> message) {
    // This is a response to a path query we theoretically made.
    // In the future we may want to check that this was actually from our DS and for a query we actually made.

    // figure out how many bytes the path query is
    quint16 numPathBytes;
    message->readPrimitive(&numPathBytes);

    // pull the path from the packet
    if (message->getBytesLeftToRead() < numPathBytes) {
        qCDebug(networking) << "Could not read query path from DomainServerPathQueryResponse. Bailing.";
        return;
    }
    
    QString pathQuery = QString::fromUtf8(message->getRawMessage() + message->getPosition(), numPathBytes);
    message->seek(message->getPosition() + numPathBytes);

    // figure out how many bytes the viewpoint is
    quint16 numViewpointBytes;
    message->readPrimitive(&numViewpointBytes);

    if (message->getBytesLeftToRead() < numViewpointBytes) {
        qCDebug(networking) << "Could not read resulting viewpoint from DomainServerPathQueryReponse. Bailing";
        return;
    }
    
    // pull the viewpoint from the packet
    QString viewpoint = QString::fromUtf8(message->getRawMessage() + message->getPosition(), numViewpointBytes);
    
    // Hand it off to the AddressManager so it can handle it as a relative viewpoint
    if (DependencyManager::get<AddressManager>()->goToViewpointForPath(viewpoint, pathQuery)) {
        qCDebug(networking) << "Going to viewpoint" << viewpoint << "which was the lookup result for path" << pathQuery;
    } else {
        qCDebug(networking) << "Could not go to viewpoint" << viewpoint
            << "which was the lookup result for path" << pathQuery;
    }
}

void NodeList::handleICEConnectionToDomainServer() {
    // if we're still waiting to get sockets we want to ping for the domain-server
    // then send another heartbeat now
    if (!_domainHandler.getICEPeer().hasSockets()) {

        _domainHandler.getICEPeer().resetConnectionAttempts();

        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendICEServerQuery);

        LimitedNodeList::sendPeerQueryToIceServer(_domainHandler.getICEServerSockAddr(),
                                                  _domainHandler.getICEClientID(),
                                                  _domainHandler.getPendingDomainID());
    }
}

void NodeList::pingPunchForDomainServer() {
    // make sure if we're here that we actually still need to ping the domain-server
    if (_domainHandler.getIP().isNull() && _domainHandler.getICEPeer().hasSockets()) {

        // check if we've hit the number of pings we'll send to the DS before we consider it a fail
        const int NUM_DOMAIN_SERVER_PINGS_BEFORE_RESET = 2000 / UDP_PUNCH_PING_INTERVAL_MS;

        if (_domainHandler.getICEPeer().getConnectionAttempts() == 0) {
            qCDebug(networking) << "Sending ping packets to establish connectivity with domain-server with ID"
                << uuidStringWithoutCurlyBraces(_domainHandler.getPendingDomainID());
        } else {
            if (_domainHandler.getICEPeer().getConnectionAttempts() % NUM_DOMAIN_SERVER_PINGS_BEFORE_RESET == 0) {
                // if we have then nullify the domain handler's network peer and send a fresh ICE heartbeat
                qCDebug(networking) << "No ping replies received from domain-server with ID"
                    << uuidStringWithoutCurlyBraces(_domainHandler.getICEClientID()) << "-" << "re-sending ICE query.";

                _domainHandler.getICEPeer().softReset();
                handleICEConnectionToDomainServer();

                return;
            }
        }

        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendPingsToDS);

        // send the ping packet to the local and public sockets for this node
        auto localPingPacket = constructICEPingPacket(PingType::Local, _sessionUUID);
        sendPacket(std::move(localPingPacket), _domainHandler.getICEPeer().getLocalSocket());

        auto publicPingPacket = constructICEPingPacket(PingType::Public, _sessionUUID);
        sendPacket(std::move(publicPingPacket), _domainHandler.getICEPeer().getPublicSocket());

        _domainHandler.getICEPeer().incrementConnectionAttempts();
    }
}

void NodeList::processDomainServerConnectionTokenPacket(QSharedPointer<ReceivedMessage> message) {
    if (_domainHandler.getSockAddr().isNull()) {
        // refuse to process this packet if we aren't currently connected to the DS
        return;
    }
    // read in the connection token from the packet, then send domain-server checkin
    _domainHandler.setConnectionToken(QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID)));
    sendDomainServerCheckIn();
}

void NodeList::processDomainServerList(QSharedPointer<ReceivedMessage> message) {
    if (_domainHandler.getSockAddr().isNull()) {
        qWarning() << "IGNORING DomainList packet while not connected to a Domain Server";
        // refuse to process this packet if we aren't currently connected to the DS
        return;
    }

    // this is a packet from the domain server, reset the count of un-replied check-ins
    _numNoReplyDomainCheckIns = 0;

    // emit our signal so listeners know we just heard from the DS
    emit receivedDomainServerList();

    DependencyManager::get<NodeList>()->flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::ReceiveDSList);

    QDataStream packetStream(message->getMessage());

    // grab the domain's ID from the beginning of the packet
    QUuid domainUUID;
    packetStream >> domainUUID;

    if (_domainHandler.isConnected() && _domainHandler.getUUID() != domainUUID) {
        // Recieved packet from different domain.
        qWarning() << "IGNORING DomainList packet from" << domainUUID << "while connected to" << _domainHandler.getUUID();
        return;
    }

    // pull our owner UUID from the packet, it's always the first thing
    QUuid newUUID;
    packetStream >> newUUID;
    setSessionUUID(newUUID);

    // if this was the first domain-server list from this domain, we've now connected
    if (!_domainHandler.isConnected()) {
        _domainHandler.setUUID(domainUUID);
        _domainHandler.setIsConnected(true);

        // in case we didn't use a place name to get to this domain,
        // give the address manager a chance to lookup a default one now
        DependencyManager::get<AddressManager>()->lookupShareableNameForDomainID(domainUUID);
    }

    // pull the permissions/right/privileges for this node out of the stream
    NodePermissions newPermissions;
    packetStream >> newPermissions;
    setPermissions(newPermissions);

    // pull each node in the packet
    while (packetStream.device()->pos() < message->getSize()) {
        parseNodeFromPacketStream(packetStream);
    }
}

void NodeList::processDomainServerAddedNode(QSharedPointer<ReceivedMessage> message) {
    // setup a QDataStream
    QDataStream packetStream(message->getMessage());

    // use our shared method to pull out the new node
    parseNodeFromPacketStream(packetStream);
}

void NodeList::processDomainServerRemovedNode(QSharedPointer<ReceivedMessage> message) {
    // read the UUID from the packet, remove it if it exists
    QUuid nodeUUID = QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID));
    qCDebug(networking) << "Received packet from domain-server to remove node with UUID" << uuidStringWithoutCurlyBraces(nodeUUID);
    killNodeWithUUID(nodeUUID);
}

void NodeList::parseNodeFromPacketStream(QDataStream& packetStream) {
    // setup variables to read into from QDataStream
    qint8 nodeType;
    QUuid nodeUUID, connectionUUID;
    HifiSockAddr nodePublicSocket, nodeLocalSocket;
    NodePermissions permissions;

    packetStream >> nodeType >> nodeUUID >> nodePublicSocket >> nodeLocalSocket >> permissions;

    // if the public socket address is 0 then it's reachable at the same IP
    // as the domain server
    if (nodePublicSocket.getAddress().isNull()) {
        nodePublicSocket.setAddress(_domainHandler.getIP());
    }

    packetStream >> connectionUUID;

    SharedNodePointer node = addOrUpdateNode(nodeUUID, nodeType, nodePublicSocket,
                                             nodeLocalSocket, permissions, connectionUUID);
}

void NodeList::sendAssignment(Assignment& assignment) {
 
    PacketType assignmentPacketType = assignment.getCommand() == Assignment::CreateCommand
        ? PacketType::CreateAssignment
        : PacketType::RequestAssignment;

    auto assignmentPacket = NLPacket::create(assignmentPacketType);
    
    QDataStream packetStream(assignmentPacket.get());
    packetStream << assignment;
    
    sendPacket(std::move(assignmentPacket), _assignmentServerSocket);
}

void NodeList::pingPunchForInactiveNode(const SharedNodePointer& node) {
    if (node->getType() == NodeType::AudioMixer) {
        flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SendAudioPing);
    }

    // every second we're trying to ping this node and we're not getting anywhere - debug that out
    const int NUM_DEBUG_CONNECTION_ATTEMPTS = 1000 / (UDP_PUNCH_PING_INTERVAL_MS);

    if (node->getConnectionAttempts() > 0 && node->getConnectionAttempts() % NUM_DEBUG_CONNECTION_ATTEMPTS == 0) {
        qCDebug(networking) << "No response to UDP hole punch pings for node" << node->getUUID() << "in last second.";
    }

    // send the ping packet to the local and public sockets for this node
    auto localPingPacket = constructPingPacket(PingType::Local);
    sendPacket(std::move(localPingPacket), *node, node->getLocalSocket());

    auto publicPingPacket = constructPingPacket(PingType::Public);
    sendPacket(std::move(publicPingPacket), *node, node->getPublicSocket());

    if (!node->getSymmetricSocket().isNull()) {
        auto symmetricPingPacket = constructPingPacket(PingType::Symmetric);
        sendPacket(std::move(symmetricPingPacket), *node, node->getSymmetricSocket());
    }

    node->incrementConnectionAttempts();
}

void NodeList::startNodeHolePunch(const SharedNodePointer& node) {
    // connect to the correct signal on this node so we know when to ping it
    connect(node.data(), &Node::pingTimerTimeout, this, &NodeList::handleNodePingTimeout);

    // start the ping timer for this node
    node->startPingTimer();

    // ping this node immediately
    pingPunchForInactiveNode(node);
}

void NodeList::handleNodePingTimeout() {
    Node* senderNode = qobject_cast<Node*>(sender());
    if (senderNode) {
        SharedNodePointer sharedNode = nodeWithUUID(senderNode->getUUID());

        if (sharedNode && !sharedNode->getActiveSocket()) {
            pingPunchForInactiveNode(sharedNode);
        }
    }
}

void NodeList::activateSocketFromNodeCommunication(ReceivedMessage& message, const SharedNodePointer& sendingNode) {
    // deconstruct this ping packet to see if it is a public or local reply
    QDataStream packetStream(message.getMessage());

    quint8 pingType;
    packetStream >> pingType;

    // if this is a local or public ping then we can activate a socket
    // we do nothing with agnostic pings, those are simply for timing
    if (pingType == PingType::Local && sendingNode->getActiveSocket() != &sendingNode->getLocalSocket()) {
        sendingNode->activateLocalSocket();
    } else if (pingType == PingType::Public && !sendingNode->getActiveSocket()) {
        sendingNode->activatePublicSocket();
    } else if (pingType == PingType::Symmetric && !sendingNode->getActiveSocket()) {
        sendingNode->activateSymmetricSocket();
    }

    if (sendingNode->getType() == NodeType::AudioMixer) {
       flagTimeForConnectionStep(LimitedNodeList::ConnectionStep::SetAudioMixerSocket);
    }
}

void NodeList::stopKeepalivePingTimer() {
    if (_keepAlivePingTimer.isActive()) {
        _keepAlivePingTimer.stop();
    }
}

void NodeList::sendKeepAlivePings() {
    eachMatchingNode([this](const SharedNodePointer& node)->bool {
        return _nodeTypesOfInterest.contains(node->getType());
    }, [&](const SharedNodePointer& node) {
        sendPacket(constructPingPacket(), *node);
    });
}

bool NodeList::sockAddrBelongsToDomainOrNode(const HifiSockAddr& sockAddr) {
    return _domainHandler.getSockAddr() == sockAddr || LimitedNodeList::sockAddrBelongsToNode(sockAddr);
}

void NodeList::ignoreNodesInRadius(bool enabled) {
    bool isEnabledChange = _ignoreRadiusEnabled.get() != enabled;
    _ignoreRadiusEnabled.set(enabled);

    eachMatchingNode([](const SharedNodePointer& node)->bool {
        return (node->getType() == NodeType::AudioMixer || node->getType() == NodeType::AvatarMixer);
    }, [this](const SharedNodePointer& destinationNode) {
        sendIgnoreRadiusStateToNode(destinationNode);
    });
    if (isEnabledChange) {
        emit ignoreRadiusEnabledChanged(enabled);
    }
}

void NodeList::sendIgnoreRadiusStateToNode(const SharedNodePointer& destinationNode) {
    auto ignorePacket = NLPacket::create(PacketType::RadiusIgnoreRequest, sizeof(bool) + sizeof(float), true);
    ignorePacket->writePrimitive(_ignoreRadiusEnabled.get());
    sendPacket(std::move(ignorePacket), *destinationNode);
}

void NodeList::ignoreNodeBySessionID(const QUuid& nodeID, bool ignoreEnabled) {
    // enumerate the nodes to send a reliable ignore packet to each that can leverage it
    if (!nodeID.isNull() && _sessionUUID != nodeID) {
        eachMatchingNode([&nodeID](const SharedNodePointer& node)->bool {
            if (node->getType() == NodeType::AudioMixer || node->getType() == NodeType::AvatarMixer) {
                return true;
            } else {
                return false;
            }
        }, [&nodeID, ignoreEnabled, this](const SharedNodePointer& destinationNode) {
            // create a reliable NLPacket with space for the ignore UUID
            auto ignorePacket = NLPacket::create(PacketType::NodeIgnoreRequest, NUM_BYTES_RFC4122_UUID + sizeof(bool), true);

            ignorePacket->writePrimitive(ignoreEnabled);
            // write the node ID to the packet
            ignorePacket->write(nodeID.toRfc4122());

            qCDebug(networking) << "Sending packet to" << (destinationNode->getType() == NodeType::AudioMixer ? "AudioMixer" : "AvatarMixer") << "to"
                << (ignoreEnabled ? "ignore" : "unignore") << "node" << uuidStringWithoutCurlyBraces(nodeID);

            // send off this ignore packet reliably to the matching node
            sendPacket(std::move(ignorePacket), *destinationNode);
        });

        if (ignoreEnabled) {
            QReadLocker ignoredSetLocker{ &_ignoredSetLock }; // read lock for insert
            QReadLocker personalMutedSetLocker{ &_personalMutedSetLock }; // read lock for insert 
            // add this nodeID to our set of ignored IDs
            _ignoredNodeIDs.insert(nodeID);
            // add this nodeID to our set of personal muted IDs
            _personalMutedNodeIDs.insert(nodeID);
            emit ignoredNode(nodeID, true);
        } else {
            QWriteLocker ignoredSetLocker{ &_ignoredSetLock }; // write lock for unsafe_erase
            QWriteLocker personalMutedSetLocker{ &_personalMutedSetLock }; // write lock for unsafe_erase
            _ignoredNodeIDs.unsafe_erase(nodeID);
            _personalMutedNodeIDs.unsafe_erase(nodeID);
            emit ignoredNode(nodeID, false);
        }

    } else {
        qWarning() << "NodeList::ignoreNodeBySessionID called with an invalid ID or an ID which matches the current session ID.";
    }
}

bool NodeList::isIgnoringNode(const QUuid& nodeID) const {
    QReadLocker ignoredSetLocker{ &_ignoredSetLock };
    return _ignoredNodeIDs.find(nodeID) != _ignoredNodeIDs.cend();
}

void NodeList::personalMuteNodeBySessionID(const QUuid& nodeID, bool muteEnabled) {
    // cannot personal mute yourself, or nobody
    if (!nodeID.isNull() && _sessionUUID != nodeID) {
        auto audioMixer = soloNodeOfType(NodeType::AudioMixer);
        if (audioMixer) {
            if (isIgnoringNode(nodeID)) {
                qCDebug(networking) << "You can't personally mute or unmute a node you're already ignoring.";
            }
            else {
                // setup the packet
                auto personalMutePacket = NLPacket::create(PacketType::NodeIgnoreRequest, NUM_BYTES_RFC4122_UUID + sizeof(bool), true);

                personalMutePacket->writePrimitive(muteEnabled);
                // write the node ID to the packet
                personalMutePacket->write(nodeID.toRfc4122());

                qCDebug(networking) << "Sending Personal Mute Packet to" << (muteEnabled ? "mute" : "unmute") << "node" << uuidStringWithoutCurlyBraces(nodeID);

                sendPacket(std::move(personalMutePacket), *audioMixer);


                if (muteEnabled) {
                    QReadLocker personalMutedSetLocker{ &_personalMutedSetLock }; // read lock for insert 
                    // add this nodeID to our set of personal muted IDs
                    _personalMutedNodeIDs.insert(nodeID);
                } else {
                    QWriteLocker personalMutedSetLocker{ &_personalMutedSetLock }; // write lock for unsafe_erase
                    _personalMutedNodeIDs.unsafe_erase(nodeID);
                }
            }
        } else {
            qWarning() << "Couldn't find audio mixer to send node personal mute request";
        }
    } else {
        qWarning() << "NodeList::personalMuteNodeBySessionID called with an invalid ID or an ID which matches the current session ID.";
    }
}

bool NodeList::isPersonalMutingNode(const QUuid& nodeID) const {
    QReadLocker personalMutedSetLocker{ &_personalMutedSetLock };
    return _personalMutedNodeIDs.find(nodeID) != _personalMutedNodeIDs.cend();
}

void NodeList::maybeSendIgnoreSetToNode(SharedNodePointer newNode) {
    if (newNode->getType() == NodeType::AudioMixer) {
        // this is a mixer that we just added - it's unlikely it knows who we were previously ignoring in this session,
        // so send that list along now (assuming it isn't empty)

        QReadLocker personalMutedSetLocker{ &_personalMutedSetLock };

        if (_personalMutedNodeIDs.size() > 0) {
            // setup a packet list so we can send the stream of ignore IDs
            auto personalMutePacketList = NLPacketList::create(PacketType::NodeIgnoreRequest, QByteArray(), true);

            // Force the "enabled" flag in this packet to true
            personalMutePacketList->writePrimitive(true);

            // enumerate the ignored IDs and write them to the packet list
            auto it = _personalMutedNodeIDs.cbegin();
            while (it != _personalMutedNodeIDs.end()) {
                personalMutePacketList->write(it->toRfc4122());
                ++it;
            }

            // send this NLPacketList to the new node
            sendPacketList(std::move(personalMutePacketList), *newNode);
        }

        // also send them the current ignore radius state.
        sendIgnoreRadiusStateToNode(newNode);
    }
    if (newNode->getType() == NodeType::AvatarMixer) {
        // this is a mixer that we just added - it's unlikely it knows who we were previously ignoring in this session,
        // so send that list along now (assuming it isn't empty)

        QReadLocker ignoredSetLocker{ &_ignoredSetLock };

        if (_ignoredNodeIDs.size() > 0) {
            // setup a packet list so we can send the stream of ignore IDs
            auto ignorePacketList = NLPacketList::create(PacketType::NodeIgnoreRequest, QByteArray(), true);

            // Force the "enabled" flag in this packet to true
            ignorePacketList->writePrimitive(true);

            // enumerate the ignored IDs and write them to the packet list
            auto it = _ignoredNodeIDs.cbegin();
            while (it != _ignoredNodeIDs.end()) {
                ignorePacketList->write(it->toRfc4122());
                ++it;
            }

            // send this NLPacketList to the new node
            sendPacketList(std::move(ignorePacketList), *newNode);
        }

        // also send them the current ignore radius state.
        sendIgnoreRadiusStateToNode(newNode);
    }
}

void NodeList::kickNodeBySessionID(const QUuid& nodeID) {
    // send a request to domain-server to kick the node with the given session ID
    // the domain-server will handle the persistence of the kick (via username or IP)

    if (!nodeID.isNull() && _sessionUUID != nodeID ) {
        if (getThisNodeCanKick()) {
            // setup the packet
            auto kickPacket = NLPacket::create(PacketType::NodeKickRequest, NUM_BYTES_RFC4122_UUID, true);

            // write the node ID to the packet
            kickPacket->write(nodeID.toRfc4122());

            qCDebug(networking) << "Sending packet to kick node" << uuidStringWithoutCurlyBraces(nodeID);

            sendPacket(std::move(kickPacket), _domainHandler.getSockAddr());
        } else {
            qWarning() << "You do not have permissions to kick in this domain."
                << "Request to kick node" << uuidStringWithoutCurlyBraces(nodeID) << "will not be sent";
        }
    } else {
        qWarning() << "NodeList::kickNodeBySessionID called with an invalid ID or an ID which matches the current session ID.";

    }
}

void NodeList::muteNodeBySessionID(const QUuid& nodeID) {
    // cannot mute yourself, or nobody
    if (!nodeID.isNull() && _sessionUUID != nodeID ) {
        if (getThisNodeCanKick()) {
            auto audioMixer = soloNodeOfType(NodeType::AudioMixer);
            if (audioMixer) {
                // setup the packet
                auto mutePacket = NLPacket::create(PacketType::NodeMuteRequest, NUM_BYTES_RFC4122_UUID, true);

                // write the node ID to the packet
                mutePacket->write(nodeID.toRfc4122());

                qCDebug(networking) << "Sending packet to mute node" << uuidStringWithoutCurlyBraces(nodeID);
            
                sendPacket(std::move(mutePacket), *audioMixer);
            } else {
                qWarning() << "Couldn't find audio mixer to send node mute request";
            }
        } else {
            qWarning() << "You do not have permissions to mute in this domain."
                << "Request to mute node" << uuidStringWithoutCurlyBraces(nodeID) << "will not be sent";
        }
    } else {
        qWarning() << "NodeList::muteNodeBySessionID called with an invalid ID or an ID which matches the current session ID.";

    }
}

void NodeList::requestUsernameFromSessionID(const QUuid& nodeID) {
    // send a request to domain-server to get the username associated with the given session ID
    if (getThisNodeCanKick() || nodeID.isNull()) {
        // setup the packet
        auto usernameFromIDRequestPacket = NLPacket::create(PacketType::UsernameFromIDRequest, NUM_BYTES_RFC4122_UUID, true);

        // write the node ID to the packet
        if (nodeID.isNull()) {
            usernameFromIDRequestPacket->write(getSessionUUID().toRfc4122());
        } else {
            usernameFromIDRequestPacket->write(nodeID.toRfc4122());
        }

        qCDebug(networking) << "Sending packet to get username of node" << uuidStringWithoutCurlyBraces(nodeID);

        sendPacket(std::move(usernameFromIDRequestPacket), _domainHandler.getSockAddr());
    } else {
        qWarning() << "You do not have permissions to kick in this domain."
            << "Request to get the username of node" << uuidStringWithoutCurlyBraces(nodeID) << "will not be sent";
    }
}

void NodeList::processUsernameFromIDReply(QSharedPointer<ReceivedMessage> message) {
    // read the UUID from the packet
    QString nodeUUIDString = (QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID))).toString();
    // read the username from the packet
    QString username = message->readString();
    // read the machine fingerprint from the packet
    QString machineFingerprintString = (QUuid::fromRfc4122(message->readWithoutCopy(NUM_BYTES_RFC4122_UUID))).toString();

    qCDebug(networking) << "Got username" << username << "and machine fingerprint" << machineFingerprintString << "for node" << nodeUUIDString;

    emit usernameFromIDReply(nodeUUIDString, username, machineFingerprintString);
}

void NodeList::setRequestsDomainListData(bool isRequesting) {
    // Tell the avatar mixer whether I want to receive any additional data to which I might be entitled
    if (_requestsDomainListData == isRequesting) {
        return;
    }
    eachMatchingNode([](const SharedNodePointer& node)->bool {
        return node->getType() == NodeType::AvatarMixer;
    }, [this, isRequesting](const SharedNodePointer& destinationNode) {
        auto packet = NLPacket::create(PacketType::RequestsDomainListData, sizeof(bool), true); // reliable
        packet->writePrimitive(isRequesting);
        sendPacket(std::move(packet), *destinationNode);
    });
    _requestsDomainListData = isRequesting;
}
