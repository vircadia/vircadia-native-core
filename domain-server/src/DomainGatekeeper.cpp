//
//  DomainGatekeeper.cpp
//  domain-server/src
//
//  Created by Stephen Birarda on 2015-08-24.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainGatekeeper.h"

#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <AccountManager.h>
#include <Assignment.h>
#include <JSONBreakableMarshal.h>

#include "DomainServer.h"
#include "DomainServerNodeData.h"

using SharedAssignmentPointer = QSharedPointer<Assignment>;

DomainGatekeeper::DomainGatekeeper(DomainServer* server) :
    _server(server)
{
    
}

void DomainGatekeeper::addPendingAssignedNode(const QUuid& nodeUUID, const QUuid& assignmentUUID,
                                              const QUuid& walletUUID, const QString& nodeVersion) {
    _pendingAssignedNodes.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(nodeUUID),
                                  std::forward_as_tuple(assignmentUUID, walletUUID, nodeVersion));
}

QUuid DomainGatekeeper::assignmentUUIDForPendingAssignment(const QUuid& tempUUID) {
    auto it = _pendingAssignedNodes.find(tempUUID);
    
    if (it != _pendingAssignedNodes.end()) {
        return it->second.getAssignmentUUID();
    } else {
        return QUuid();
    }
}

const NodeSet STATICALLY_ASSIGNED_NODES = NodeSet() << NodeType::AudioMixer
    << NodeType::AvatarMixer << NodeType::EntityServer
    << NodeType::AssetServer;

void DomainGatekeeper::processConnectRequestPacket(QSharedPointer<NLPacket> packet) {
    if (packet->getPayloadSize() == 0) {
        return;
    }
    
    QDataStream packetStream(packet.data());
    
    // read a NodeConnectionData object from the packet so we can pass around this data while we're inspecting it
    NodeConnectionData nodeConnection = NodeConnectionData::fromDataStream(packetStream, packet->getSenderSockAddr());
    
    if (nodeConnection.localSockAddr.isNull() || nodeConnection.publicSockAddr.isNull()) {
        qDebug() << "Unexpected data received for node local socket or public socket. Will not allow connection.";
        return;
    }
    
    static const NodeSet VALID_NODE_TYPES {
        NodeType::AudioMixer, NodeType::AvatarMixer, NodeType::AssetServer, NodeType::EntityServer, NodeType::Agent
    };
    
    if (!VALID_NODE_TYPES.contains(nodeConnection.nodeType)) {
        qDebug() << "Received an invalid node type with connect request. Will not allow connection from"
            << nodeConnection.senderSockAddr;
        return;
    }
    
    // check if this connect request matches an assignment in the queue
    auto pendingAssignment = _pendingAssignedNodes.find(nodeConnection.connectUUID);
    
    SharedNodePointer node;
    
    if (pendingAssignment != _pendingAssignedNodes.end()) {
        node = processAssignmentConnectRequest(nodeConnection, pendingAssignment->second);
    } else if (!STATICALLY_ASSIGNED_NODES.contains(nodeConnection.nodeType)) {
        QString username;
        QByteArray usernameSignature;
        
        if (packet->bytesLeftToRead() > 0) {
            // read username from packet
            packetStream >> username;
            
            if (packet->bytesLeftToRead() > 0) {
                // read user signature from packet
                packetStream >> usernameSignature;
            }
        }
        
        node = processAgentConnectRequest(nodeConnection, username, usernameSignature);
    }
    
    if (node) {
        // set the sending sock addr and node interest set on this node
        DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());
        nodeData->setSendingSockAddr(packet->getSenderSockAddr());
        nodeData->setNodeInterestSet(nodeConnection.interestList.toSet());
        
        // signal that we just connected a node so the DomainServer can get it a list
        // and broadcast its presence right away
        emit connectedNode(node);
    } else {
        qDebug() << "Refusing connection from node at" << packet->getSenderSockAddr();
    }
}

SharedNodePointer DomainGatekeeper::processAssignmentConnectRequest(const NodeConnectionData& nodeConnection,
                                                                    const PendingAssignedNodeData& pendingAssignment) {
    
    // make sure this matches an assignment the DS told us we sent out
    auto it = _pendingAssignedNodes.find(nodeConnection.connectUUID);
    
    SharedAssignmentPointer matchingQueuedAssignment = SharedAssignmentPointer();
    
    if (it != _pendingAssignedNodes.end()) {
        // find the matching queued static assignment in DS queue
        matchingQueuedAssignment = _server->dequeueMatchingAssignment(it->second.getAssignmentUUID(), nodeConnection.nodeType);
        
        if (matchingQueuedAssignment) {
            qDebug() << "Assignment deployed with" << uuidStringWithoutCurlyBraces(nodeConnection.connectUUID)
                << "matches unfulfilled assignment"
                << uuidStringWithoutCurlyBraces(matchingQueuedAssignment->getUUID());
        } else {
            // this is a node connecting to fulfill an assignment that doesn't exist
            // don't reply back to them so they cycle back and re-request an assignment
            qDebug() << "No match for assignment deployed with" << uuidStringWithoutCurlyBraces(nodeConnection.connectUUID);
            return SharedNodePointer();
        }
    } else {
        qDebug() << "No assignment was deployed with UUID" << uuidStringWithoutCurlyBraces(nodeConnection.connectUUID);
        return SharedNodePointer();
    }
    
    // add the new node
    SharedNodePointer newNode = addVerifiedNodeFromConnectRequest(nodeConnection);
    
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(newNode->getLinkedData());
    
    // set assignment related data on the linked data for this node
    nodeData->setAssignmentUUID(matchingQueuedAssignment->getUUID());
    nodeData->setWalletUUID(it->second.getWalletUUID());
    nodeData->setNodeVersion(it->second.getNodeVersion());
    
    // cleanup the PendingAssignedNodeData for this assignment now that it's connecting
    _pendingAssignedNodes.erase(it);
    
    // always allow assignment clients to create and destroy entities
    newNode->setCanAdjustLocks(true);
    newNode->setCanRez(true);
    
    return newNode;
}

const QString MAXIMUM_USER_CAPACITY = "security.maximum_user_capacity";
const QString ALLOWED_EDITORS_SETTINGS_KEYPATH = "security.allowed_editors";
const QString EDITORS_ARE_REZZERS_KEYPATH = "security.editors_are_rezzers";

SharedNodePointer DomainGatekeeper::processAgentConnectRequest(const NodeConnectionData& nodeConnection,
                                                               const QString& username,
                                                               const QByteArray& usernameSignature) {
    
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
    
    bool isRestrictingAccess =
        _server->_settingsManager.valueOrDefaultValueForKeyPath(RESTRICTED_ACCESS_SETTINGS_KEYPATH).toBool();
    
    // check if this user is on our local machine - if this is true they are always allowed to connect
    QHostAddress senderHostAddress = nodeConnection.senderSockAddr.getAddress();
    bool isLocalUser =
        (senderHostAddress == limitedNodeList->getLocalSockAddr().getAddress() || senderHostAddress == QHostAddress::LocalHost);
    
    // if we're using restricted access and this user is not local make sure we got a user signature
    if (isRestrictingAccess && !isLocalUser) {
        if (!username.isEmpty()) {
            if (usernameSignature.isEmpty()) {
                // if user didn't include usernameSignature in connect request, send a connectionToken packet
                sendConnectionTokenPacket(username, nodeConnection.senderSockAddr);
                
                // ask for their public key right now to make sure we have it
                requestUserPublicKey(username);
                
                return SharedNodePointer();
            }
        }
    }
    
    bool verifiedUsername = false;
    
    // if we do not have a local user we need to subject them to our verification and capacity checks
    if (!isLocalUser) {
        
        // check if we need to look at the username signature
        if (isRestrictingAccess) {
            if (isVerifiedAllowedUser(username, usernameSignature, nodeConnection.senderSockAddr)) {
                // we verified the user via their username and signature - set the verifiedUsername
                // so we don't re-decrypt their sig if we're trying to exempt them from max capacity check (due to
                // being in the allowed editors list)
                verifiedUsername = true;
            } else {
                // failed to verify user - return a null shared ptr
                return SharedNodePointer();
            }
        }
        
        if (!isWithinMaxCapacity(username, usernameSignature, verifiedUsername, nodeConnection.senderSockAddr)) {
            // we can't allow this user to connect because we are at max capacity (and they either aren't an allowed editor
            // or couldn't be verified as one)
            return SharedNodePointer();
        }
    }
    
    // if this user is in the editors list (or if the editors list is empty) set the user's node's canAdjustLocks to true
    const QVariant* allowedEditorsVariant =
        valueForKeyPath(_server->_settingsManager.getSettingsMap(), ALLOWED_EDITORS_SETTINGS_KEYPATH);
    QStringList allowedEditors = allowedEditorsVariant ? allowedEditorsVariant->toStringList() : QStringList();
    
    // if the allowed editors list is empty then everyone can adjust locks
    bool canAdjustLocks = allowedEditors.empty();
    
    if (allowedEditors.contains(username)) {
        // we have a non-empty allowed editors list - check if this user is verified to be in it
        if (!verifiedUsername) {
            if (!verifyUserSignature(username, usernameSignature, HifiSockAddr())) {
                // failed to verify a user that is in the allowed editors list
                
                // TODO: fix public key refresh in interface/metaverse and force this check
                qDebug() << "Could not verify user" << username << "as allowed editor. In the interim this user"
                    << "will be given edit rights to avoid a thrasing of public key requests and connect requests.";
            }
            
            canAdjustLocks = true;
        } else {
            // already verified this user and they are in the allowed editors list
            canAdjustLocks = true;
        }
    }
    
    // check if only editors should be able to rez entities
    const QVariant* editorsAreRezzersVariant =
        valueForKeyPath(_server->_settingsManager.getSettingsMap(), EDITORS_ARE_REZZERS_KEYPATH);
    
    bool onlyEditorsAreRezzers = false;
    if (editorsAreRezzersVariant) {
        onlyEditorsAreRezzers = editorsAreRezzersVariant->toBool();
    }
    
    bool canRez = true;
    if (onlyEditorsAreRezzers) {
        canRez = canAdjustLocks;
    }
    
    // add the new node
    SharedNodePointer newNode = addVerifiedNodeFromConnectRequest(nodeConnection);
    
    // set the edit rights for this user
    newNode->setCanAdjustLocks(canAdjustLocks);
    newNode->setCanRez(canRez);
    
    // grab the linked data for our new node so we can set the username
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(newNode->getLinkedData());
    
    // if we have a username from the connect request, set it on the DomainServerNodeData
    nodeData->setUsername(username);
    
    // also add an interpolation to JSONBreakableMarshal so that servers can get username in stats
    JSONBreakableMarshal::addInterpolationForKey(USERNAME_UUID_REPLACEMENT_STATS_KEY,
                                                 uuidStringWithoutCurlyBraces(newNode->getUUID()), username);
    
    return newNode;
}

SharedNodePointer DomainGatekeeper::addVerifiedNodeFromConnectRequest(const NodeConnectionData& nodeConnection) {
    HifiSockAddr discoveredSocket = nodeConnection.senderSockAddr;
    SharedNetworkPeer connectedPeer = _icePeers.value(nodeConnection.connectUUID);
    
    QUuid nodeUUID;
    
    if (connectedPeer) {
        //  this user negotiated a connection with us via ICE, so re-use their ICE client ID
        nodeUUID = nodeConnection.connectUUID;
        
        if (connectedPeer->getActiveSocket()) {
            // set their discovered socket to whatever the activated socket on the network peer object was
            discoveredSocket = *connectedPeer->getActiveSocket();
        }
    } else {
        // we got a connectUUID we didn't recognize, just add the node with a new UUID
        nodeUUID = QUuid::createUuid();
    }
    
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
    
    SharedNodePointer newNode = limitedNodeList->addOrUpdateNode(nodeUUID, nodeConnection.nodeType,
                                                                 nodeConnection.publicSockAddr, nodeConnection.localSockAddr);
    
    // So that we can send messages to this node at will - we need to activate the correct socket on this node now
    newNode->activateMatchingOrNewSymmetricSocket(discoveredSocket);
    
    return newNode;
}

bool DomainGatekeeper::verifyUserSignature(const QString& username,
                                           const QByteArray& usernameSignature,
                                           const HifiSockAddr& senderSockAddr) {
    
    // it's possible this user can be allowed to connect, but we need to check their username signature
    QByteArray publicKeyArray = _userPublicKeys.value(username);
    
    const QUuid& connectionToken = _connectionTokenHash.value(username.toLower());
    
    if (!publicKeyArray.isEmpty() && !connectionToken.isNull()) {
        // if we do have a public key for the user, check for a signature match
        
        const unsigned char* publicKeyData = reinterpret_cast<const unsigned char*>(publicKeyArray.constData());
        
        // first load up the public key into an RSA struct
        RSA* rsaPublicKey = d2i_RSA_PUBKEY(NULL, &publicKeyData, publicKeyArray.size());
        
        QByteArray lowercaseUsername = username.toLower().toUtf8();
        QByteArray usernameWithToken = QCryptographicHash::hash(lowercaseUsername.append(connectionToken.toRfc4122()),
                                                                QCryptographicHash::Sha256);
        
        if (rsaPublicKey) {
            QByteArray decryptedArray(RSA_size(rsaPublicKey), 0);
            int decryptResult = RSA_verify(NID_sha256,
                                           reinterpret_cast<const unsigned char*>(usernameWithToken.constData()),
                                           usernameWithToken.size(),
                                           reinterpret_cast<const unsigned char*>(usernameSignature.constData()),
                                           usernameSignature.size(),
                                           rsaPublicKey);
            
            if (decryptResult == 1) {
                qDebug() << "Username signature matches for" << username << "- allowing connection.";
                
                // free up the public key and remove connection token before we return
                RSA_free(rsaPublicKey);
                _connectionTokenHash.remove(username);
                
                return true;
                
            } else {
                if (!senderSockAddr.isNull()) {
                    qDebug() << "Error decrypting username signature for " << username << "- denying connection.";
                    sendConnectionDeniedPacket("Error decrypting username signature.", senderSockAddr);
                }
                
                // free up the public key, we don't need it anymore
                RSA_free(rsaPublicKey);
            }
            
        } else {
            
            // we can't let this user in since we couldn't convert their public key to an RSA key we could use
            if (!senderSockAddr.isNull()) {
                qDebug() << "Couldn't convert data to RSA key for" << username << "- denying connection.";
                sendConnectionDeniedPacket("Couldn't convert data to RSA key.", senderSockAddr);
            }
        }
    } else {
        if (!senderSockAddr.isNull()) {
            qDebug() << "Insufficient data to decrypt username signature - denying connection.";
            sendConnectionDeniedPacket("Insufficient data", senderSockAddr);
        }
    }
    
    requestUserPublicKey(username); // no joy.  maybe next time?
    return false;
}

bool DomainGatekeeper::isVerifiedAllowedUser(const QString& username, const QByteArray& usernameSignature,
                                             const HifiSockAddr& senderSockAddr) {
    
    if (username.isEmpty()) {
        qDebug() << "Connect request denied - no username provided.";
        
        sendConnectionDeniedPacket("No username provided", senderSockAddr);
        
        return false;
    }
    
    QStringList allowedUsers =
        _server->_settingsManager.valueOrDefaultValueForKeyPath(ALLOWED_USERS_SETTINGS_KEYPATH).toStringList();
    
    if (allowedUsers.contains(username, Qt::CaseInsensitive)) {
        if (!verifyUserSignature(username, usernameSignature, senderSockAddr)) {
            return false;
        }
    } else {
        qDebug() << "Connect request denied for user" << username << "- not in allowed users list.";
        sendConnectionDeniedPacket("User not on whitelist.", senderSockAddr);
        
        return false;
    }
    
    return true;
}

bool DomainGatekeeper::isWithinMaxCapacity(const QString& username, const QByteArray& usernameSignature,
                                           bool& verifiedUsername,
                                           const HifiSockAddr& senderSockAddr) {
    // find out what our maximum capacity is
    const QVariant* maximumUserCapacityVariant = valueForKeyPath(_server->_settingsManager.getSettingsMap(), MAXIMUM_USER_CAPACITY);
    unsigned int maximumUserCapacity = maximumUserCapacityVariant ? maximumUserCapacityVariant->toUInt() : 0;
    
    if (maximumUserCapacity > 0) {
        unsigned int connectedUsers = _server->countConnectedUsers();
        
        if (connectedUsers >= maximumUserCapacity) {
            // too many users, deny the new connection unless this user is an allowed editor
            
            const QVariant* allowedEditorsVariant =
                valueForKeyPath(_server->_settingsManager.getSettingsMap(), ALLOWED_EDITORS_SETTINGS_KEYPATH);
            
            QStringList allowedEditors = allowedEditorsVariant ? allowedEditorsVariant->toStringList() : QStringList();
            if (allowedEditors.contains(username)) {
                if (verifiedUsername || verifyUserSignature(username, usernameSignature, senderSockAddr)) {
                    verifiedUsername = true;
                    qDebug() << "Above maximum capacity -" << connectedUsers << "/" << maximumUserCapacity <<
                        "but user" << username << "is in allowed editors list so will be allowed to connect.";
                    return true;
                }
            }
            
            // deny connection from this user
            qDebug() << connectedUsers << "/" << maximumUserCapacity << "users connected, denying new connection.";
            sendConnectionDeniedPacket("Too many connected users.", senderSockAddr);
            
            return false;
        }
        
        qDebug() << connectedUsers << "/" << maximumUserCapacity << "users connected, allowing new connection.";
    }
    
    return true;
}


void DomainGatekeeper::preloadAllowedUserPublicKeys() {
    const QVariant* allowedUsersVariant = valueForKeyPath(_server->_settingsManager.getSettingsMap(), ALLOWED_USERS_SETTINGS_KEYPATH);
    QStringList allowedUsers = allowedUsersVariant ? allowedUsersVariant->toStringList() : QStringList();
    
    if (allowedUsers.size() > 0) {
        // in the future we may need to limit how many requests here - for now assume that lists of allowed users are not
        // going to create > 100 requests
        foreach(const QString& username, allowedUsers) {
            requestUserPublicKey(username);
        }
    }
}

void DomainGatekeeper::requestUserPublicKey(const QString& username) {
    // even if we have a public key for them right now, request a new one in case it has just changed
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "publicKeyJSONCallback";
    
    const QString USER_PUBLIC_KEY_PATH = "api/v1/users/%1/public_key";
    
    qDebug() << "Requesting public key for user" << username;
    
    AccountManager::getInstance().sendRequest(USER_PUBLIC_KEY_PATH.arg(username),
                                              AccountManagerAuth::None,
                                              QNetworkAccessManager::GetOperation, callbackParams);
}

void DomainGatekeeper::publicKeyJSONCallback(QNetworkReply& requestReply) {
    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    
    if (jsonObject["status"].toString() == "success") {
        // figure out which user this is for
        
        const QString PUBLIC_KEY_URL_REGEX_STRING = "api\\/v1\\/users\\/([A-Za-z0-9_\\.]+)\\/public_key";
        QRegExp usernameRegex(PUBLIC_KEY_URL_REGEX_STRING);
        
        if (usernameRegex.indexIn(requestReply.url().toString()) != -1) {
            QString username = usernameRegex.cap(1);
            
            qDebug() << "Storing a public key for user" << username;
            
            // pull the public key as a QByteArray from this response
            const QString JSON_DATA_KEY = "data";
            const QString JSON_PUBLIC_KEY_KEY = "public_key";
            
            _userPublicKeys[username] =
                QByteArray::fromBase64(jsonObject[JSON_DATA_KEY].toObject()[JSON_PUBLIC_KEY_KEY].toString().toUtf8());
        }
    }
}

void DomainGatekeeper::sendConnectionDeniedPacket(const QString& reason, const HifiSockAddr& senderSockAddr) {
    // this is an agent and we've decided we won't let them connect - send them a packet to deny connection
    QByteArray utfString = reason.toUtf8();
    quint16 payloadSize = utfString.size();
    
    // setup the DomainConnectionDenied packet
    auto connectionDeniedPacket = NLPacket::create(PacketType::DomainConnectionDenied, payloadSize + sizeof(payloadSize));
    
    // pack in the reason the connection was denied (the client displays this)
    if (payloadSize > 0) {
        connectionDeniedPacket->writePrimitive(payloadSize);
        connectionDeniedPacket->write(utfString);
    }
    
    // send the packet off
    DependencyManager::get<LimitedNodeList>()->sendPacket(std::move(connectionDeniedPacket), senderSockAddr);
}

void DomainGatekeeper::sendConnectionTokenPacket(const QString& username, const HifiSockAddr& senderSockAddr) {
    // get the existing connection token or create a new one
    QUuid& connectionToken = _connectionTokenHash[username.toLower()];
    
    if (connectionToken.isNull()) {
        connectionToken = QUuid::createUuid();
    }
    
    // setup a static connection token packet
    static auto connectionTokenPacket = NLPacket::create(PacketType::DomainServerConnectionToken, NUM_BYTES_RFC4122_UUID);
    
    // reset the packet before each time we send
    connectionTokenPacket->reset();
    
    // write the connection token
    connectionTokenPacket->write(connectionToken.toRfc4122());
    
    // send off the packet unreliably
    DependencyManager::get<LimitedNodeList>()->sendUnreliablePacket(*connectionTokenPacket, senderSockAddr);
}

const int NUM_PEER_PINGS_BEFORE_DELETE = 2000 / UDP_PUNCH_PING_INTERVAL_MS;

void DomainGatekeeper::pingPunchForConnectingPeer(const SharedNetworkPeer& peer) {
    
    if (peer->getConnectionAttempts() >= NUM_PEER_PINGS_BEFORE_DELETE) {
        // we've reached the maximum number of ping attempts
        qDebug() << "Maximum number of ping attempts reached for peer with ID" << peer->getUUID();
        qDebug() << "Removing from list of connecting peers.";
        
        _icePeers.remove(peer->getUUID());
    } else {
        auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
        
        // send the ping packet to the local and public sockets for this node
        auto localPingPacket = limitedNodeList->constructICEPingPacket(PingType::Local, limitedNodeList->getSessionUUID());
        limitedNodeList->sendPacket(std::move(localPingPacket), peer->getLocalSocket());
        
        auto publicPingPacket = limitedNodeList->constructICEPingPacket(PingType::Public, limitedNodeList->getSessionUUID());
        limitedNodeList->sendPacket(std::move(publicPingPacket), peer->getPublicSocket());
        
        peer->incrementConnectionAttempts();
    }
}

void DomainGatekeeper::handlePeerPingTimeout() {
    NetworkPeer* senderPeer = qobject_cast<NetworkPeer*>(sender());
    
    if (senderPeer) {
        SharedNetworkPeer sharedPeer = _icePeers.value(senderPeer->getUUID());
        
        if (sharedPeer && !sharedPeer->getActiveSocket()) {
            pingPunchForConnectingPeer(sharedPeer);
        }
    }
}

void DomainGatekeeper::processICEPeerInformationPacket(QSharedPointer<NLPacket> packet) {
    // loop through the packet and pull out network peers
    // any peer we don't have we add to the hash, otherwise we update
    QDataStream iceResponseStream(packet.data());
    
    NetworkPeer* receivedPeer = new NetworkPeer;
    iceResponseStream >> *receivedPeer;
    
    if (!_icePeers.contains(receivedPeer->getUUID())) {
        qDebug() << "New peer requesting ICE connection being added to hash -" << *receivedPeer;
        SharedNetworkPeer newPeer = SharedNetworkPeer(receivedPeer);
        _icePeers[receivedPeer->getUUID()] = newPeer;
        
        // make sure we know when we should ping this peer
        connect(newPeer.data(), &NetworkPeer::pingTimerTimeout, this, &DomainGatekeeper::handlePeerPingTimeout);
        
        // immediately ping the new peer, and start a timer to continue pinging it until we connect to it
        newPeer->startPingTimer();
        
        qDebug() << "Sending ping packets to establish connectivity with ICE peer with ID"
            << newPeer->getUUID();
        
        pingPunchForConnectingPeer(newPeer);
    } else {
        delete receivedPeer;
    }
}

void DomainGatekeeper::processICEPingPacket(QSharedPointer<NLPacket> packet) {
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
    auto pingReplyPacket = limitedNodeList->constructICEPingReplyPacket(*packet, limitedNodeList->getSessionUUID());
    
    limitedNodeList->sendPacket(std::move(pingReplyPacket), packet->getSenderSockAddr());
}

void DomainGatekeeper::processICEPingReplyPacket(QSharedPointer<NLPacket> packet) {
    QDataStream packetStream(packet.data());
    
    QUuid nodeUUID;
    packetStream >> nodeUUID;
    
    SharedNetworkPeer sendingPeer = _icePeers.value(nodeUUID);
    
    if (sendingPeer) {
        // we had this NetworkPeer in our connecting list - add the right sock addr to our connected list
        sendingPeer->activateMatchingOrNewSymmetricSocket(packet->getSenderSockAddr());
    }
}
