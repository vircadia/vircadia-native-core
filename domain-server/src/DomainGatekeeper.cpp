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
    << NodeType::AssetServer
    << NodeType::MessagesMixer;

void DomainGatekeeper::processConnectRequestPacket(QSharedPointer<ReceivedMessage> message) {
    if (message->getSize() == 0) {
        return;
    }
    QDataStream packetStream(message->getMessage());

    // read a NodeConnectionData object from the packet so we can pass around this data while we're inspecting it
    NodeConnectionData nodeConnection = NodeConnectionData::fromDataStream(packetStream, message->getSenderSockAddr());

    QByteArray myProtocolVersion = protocolVersionsSignature();
    if (nodeConnection.protocolVersion != myProtocolVersion) {
        sendProtocolMismatchConnectionDenial(message->getSenderSockAddr());
        return;
    }

    if (nodeConnection.localSockAddr.isNull() || nodeConnection.publicSockAddr.isNull()) {
        qDebug() << "Unexpected data received for node local socket or public socket. Will not allow connection.";
        return;
    }

    static const NodeSet VALID_NODE_TYPES {
        NodeType::AudioMixer, NodeType::AvatarMixer, NodeType::AssetServer, NodeType::EntityServer, NodeType::Agent, NodeType::MessagesMixer
    };

    if (!VALID_NODE_TYPES.contains(nodeConnection.nodeType)) {
        qDebug() << "Received an invalid node type with connect request. Will not allow connection from"
            << nodeConnection.senderSockAddr << ": " << nodeConnection.nodeType;
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

        if (message->getBytesLeftToRead() > 0) {
            // read username from packet
            packetStream >> username;

            if (message->getBytesLeftToRead() > 0) {
                // read user signature from packet
                packetStream >> usernameSignature;
            }
        }

        node = processAgentConnectRequest(nodeConnection, username, usernameSignature);
    }

    if (node) {
        // set the sending sock addr and node interest set on this node
        DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());
        nodeData->setSendingSockAddr(message->getSenderSockAddr());

        // guard against patched agents asking to hear about other agents
        auto safeInterestSet = nodeConnection.interestList.toSet();
        if (nodeConnection.nodeType == NodeType::Agent) {
            safeInterestSet.remove(NodeType::Agent);
        }

        nodeData->setNodeInterestSet(safeInterestSet);
        nodeData->setPlaceName(nodeConnection.placeName);

        qDebug() << "Allowed connection from node" << uuidStringWithoutCurlyBraces(node->getUUID())
            << "on" << message->getSenderSockAddr() << "with MAC" << nodeConnection.hardwareAddress;

        // signal that we just connected a node so the DomainServer can get it a list
        // and broadcast its presence right away
        emit connectedNode(node);
    } else {
        qDebug() << "Refusing connection from node at" << message->getSenderSockAddr()
            << "with hardware address" << nodeConnection.hardwareAddress;
    }
}

NodePermissions DomainGatekeeper::setPermissionsForUser(bool isLocalUser, QString verifiedUsername,
                                                        const QHostAddress& senderAddress, const QString& hardwareAddress) {
    NodePermissions userPerms;

    userPerms.setAll(false);

    if (isLocalUser) {
        userPerms |= _server->_settingsManager.getStandardPermissionsForName(NodePermissions::standardNameLocalhost);
#ifdef WANT_DEBUG
        qDebug() << "|  user-permissions: is local user, so:" << userPerms;
#endif
    }

    if (verifiedUsername.isEmpty()) {
        userPerms |= _server->_settingsManager.getStandardPermissionsForName(NodePermissions::standardNameAnonymous);
#ifdef WANT_DEBUG
        qDebug() << "|  user-permissions: unverified or no username for" << userPerms.getID() << ", so:" << userPerms;
#endif
        if (!hardwareAddress.isEmpty() && _server->_settingsManager.hasPermissionsForMAC(hardwareAddress)) {
            // this user comes from a MAC we have in our permissions table, apply those permissions
            userPerms = _server->_settingsManager.getPermissionsForMAC(hardwareAddress);

#ifdef WANT_DEBUG
            qDebug() << "|  user-permissions: specific MAC matches, so:" << userPerms;
#endif
        } else if (_server->_settingsManager.hasPermissionsForIP(senderAddress)) {
            // this user comes from an IP we have in our permissions table, apply those permissions
            userPerms = _server->_settingsManager.getPermissionsForIP(senderAddress);

#ifdef WANT_DEBUG
            qDebug() << "|  user-permissions: specific IP matches, so:" << userPerms;
#endif
        }
    } else {
        if (_server->_settingsManager.havePermissionsForName(verifiedUsername)) {
            userPerms = _server->_settingsManager.getPermissionsForName(verifiedUsername);
#ifdef WANT_DEBUG
            qDebug() << "|  user-permissions: specific user matches, so:" << userPerms;
#endif
        } else if (!hardwareAddress.isEmpty() && _server->_settingsManager.hasPermissionsForMAC(hardwareAddress)) {
            // this user comes from a MAC we have in our permissions table, apply those permissions
            userPerms = _server->_settingsManager.getPermissionsForMAC(hardwareAddress);

#ifdef WANT_DEBUG
            qDebug() << "|  user-permissions: specific MAC matches, so:" << userPerms;
#endif
        } else if (_server->_settingsManager.hasPermissionsForIP(senderAddress)) {
            // this user comes from an IP we have in our permissions table, apply those permissions
            userPerms = _server->_settingsManager.getPermissionsForIP(senderAddress);

#ifdef WANT_DEBUG
            qDebug() << "|  user-permissions: specific IP matches, so:" << userPerms;
#endif
        } else {
            // they are logged into metaverse, but we don't have specific permissions for them.
            userPerms |= _server->_settingsManager.getStandardPermissionsForName(NodePermissions::standardNameLoggedIn);
#ifdef WANT_DEBUG
            qDebug() << "|  user-permissions: user is logged-into metaverse, so:" << userPerms;
#endif

            // if this user is a friend of the domain-owner, give them friend's permissions
            if (_domainOwnerFriends.contains(verifiedUsername)) {
                userPerms |= _server->_settingsManager.getStandardPermissionsForName(NodePermissions::standardNameFriends);
#ifdef WANT_DEBUG
                qDebug() << "|  user-permissions: user is friends with domain-owner, so:" << userPerms;
#endif
            }

            // if this user is a known member of a group, give them the implied permissions
            foreach (QUuid groupID, _server->_settingsManager.getGroupIDs()) {
                QUuid rankID = _server->_settingsManager.isGroupMember(verifiedUsername, groupID);
                if (rankID != QUuid()) {
                    userPerms |= _server->_settingsManager.getPermissionsForGroup(groupID, rankID);

                    GroupRank rank = _server->_settingsManager.getGroupRank(groupID, rankID);
#ifdef WANT_DEBUG
                    qDebug() << "|  user-permissions: user " << verifiedUsername << "is in group:" << groupID << " rank:"
                             << rank.name << "so:" << userPerms;
#endif
                }
            }

            // if this user is a known member of a blacklist group, remove the implied permissions
            foreach (QUuid groupID, _server->_settingsManager.getBlacklistGroupIDs()) {
                QUuid rankID = _server->_settingsManager.isGroupMember(verifiedUsername, groupID);
                if (rankID != QUuid()) {
                    QUuid rankID = _server->_settingsManager.isGroupMember(verifiedUsername, groupID);
                    if (rankID != QUuid()) {
                        userPerms &= ~_server->_settingsManager.getForbiddensForGroup(groupID, rankID);

                        GroupRank rank = _server->_settingsManager.getGroupRank(groupID, rankID);
#ifdef WANT_DEBUG
                        qDebug() << "|  user-permissions: user is in blacklist group:" << groupID << " rank:" << rank.name
                                 << "so:" << userPerms;
#endif
                    }
                }
            }
        }

        userPerms.setID(verifiedUsername);
        userPerms.setVerifiedUserName(verifiedUsername);
    }

#ifdef WANT_DEBUG
    qDebug() << "|  user-permissions: final:" << userPerms;
#endif
    return userPerms;
}

void DomainGatekeeper::updateNodePermissions() {
    // If the permissions were changed on the domain-server webpage (and nothing else was), a restart isn't required --
    // we reprocess the permissions map and update the nodes here.  The node list is frequently sent out to all
    // the connected nodes, so these changes are propagated to other nodes.

    QList<SharedNodePointer> nodesToKill;

    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
    limitedNodeList->eachNode([this, limitedNodeList, &nodesToKill](const SharedNodePointer& node){
        // the id and the username in NodePermissions will often be the same, but id is set before
        // authentication and verifiedUsername is only set once they user's key has been confirmed.
        QString verifiedUsername = node->getPermissions().getVerifiedUserName();
        NodePermissions userPerms(NodePermissionsKey(verifiedUsername, 0));

        if (node->getPermissions().isAssignment) {
            // this node is an assignment-client
            userPerms.isAssignment = true;
            userPerms.permissions |= NodePermissions::Permission::canConnectToDomain;
            userPerms.permissions |= NodePermissions::Permission::canAdjustLocks;
            userPerms.permissions |= NodePermissions::Permission::canRezPermanentEntities;
            userPerms.permissions |= NodePermissions::Permission::canRezTemporaryEntities;
            userPerms.permissions |= NodePermissions::Permission::canWriteToAssetServer;
        } else {
            // this node is an agent
            const QHostAddress& addr = node->getLocalSocket().getAddress();
            bool isLocalUser = (addr == limitedNodeList->getLocalSockAddr().getAddress() ||
                                addr == QHostAddress::LocalHost);

            // at this point we don't have a sending socket for packets from this node - assume it is the active socket
            // or the public socket if we haven't activated a socket for the node yet
            HifiSockAddr connectingAddr = node->getActiveSocket() ? *node->getActiveSocket() : node->getPublicSocket();

            QString hardwareAddress;

            DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(node->getLinkedData());
            if (nodeData) {
                hardwareAddress = nodeData->getHardwareAddress();
            }

            userPerms = setPermissionsForUser(isLocalUser, verifiedUsername, connectingAddr.getAddress(), hardwareAddress);
        }

        node->setPermissions(userPerms);

        if (!userPerms.can(NodePermissions::Permission::canConnectToDomain)) {
            qDebug() << "node" << node->getUUID() << "no longer has permission to connect.";
            // hang up on this node
            nodesToKill << node;
        }
    });

    foreach (auto node, nodesToKill) {
        emit killNode(node);
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
    nodeData->setHardwareAddress(nodeConnection.hardwareAddress);
    nodeData->setWasAssigned(true);

    // cleanup the PendingAssignedNodeData for this assignment now that it's connecting
    _pendingAssignedNodes.erase(it);

    NodePermissions userPerms;
    userPerms.isAssignment = true;
    userPerms.permissions |= NodePermissions::Permission::canConnectToDomain;
    // always allow assignment clients to create and destroy entities
    userPerms.permissions |= NodePermissions::Permission::canAdjustLocks;
    userPerms.permissions |= NodePermissions::Permission::canRezPermanentEntities;
    userPerms.permissions |= NodePermissions::Permission::canRezTemporaryEntities;
    userPerms.permissions |= NodePermissions::Permission::canWriteToAssetServer;
    newNode->setPermissions(userPerms);
    return newNode;
}

const QString MAXIMUM_USER_CAPACITY = "security.maximum_user_capacity";
const QString MAXIMUM_USER_CAPACITY_REDIRECT_LOCATION = "security.maximum_user_capacity_redirect_location";

SharedNodePointer DomainGatekeeper::processAgentConnectRequest(const NodeConnectionData& nodeConnection,
                                                               const QString& username,
                                                               const QByteArray& usernameSignature) {

    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();

    // start with empty permissions
    NodePermissions userPerms(NodePermissionsKey(username, 0));
    userPerms.setAll(false);

    // check if this user is on our local machine - if this is true set permissions to those for a "localhost" connection
    QHostAddress senderHostAddress = nodeConnection.senderSockAddr.getAddress();
    bool isLocalUser =
        (senderHostAddress == limitedNodeList->getLocalSockAddr().getAddress() || senderHostAddress == QHostAddress::LocalHost);

    QString verifiedUsername; // if this remains empty, consider this an anonymous connection attempt
    if (!username.isEmpty()) {
        if (usernameSignature.isEmpty()) {
            // user is attempting to prove their identity to us, but we don't have enough information
            sendConnectionTokenPacket(username, nodeConnection.senderSockAddr);
            // ask for their public key right now to make sure we have it
            requestUserPublicKey(username);
            getGroupMemberships(username); // optimistically get started on group memberships
#ifdef WANT_DEBUG
            qDebug() << "stalling login because we have no username-signature:" << username;
#endif
            return SharedNodePointer();
        } else if (verifyUserSignature(username, usernameSignature, nodeConnection.senderSockAddr)) {
            // they sent us a username and the signature verifies it
            getGroupMemberships(username);
            verifiedUsername = username;
        } else {
            // they sent us a username, but it didn't check out
            requestUserPublicKey(username);
#ifdef WANT_DEBUG
            qDebug() << "stalling login because signature verification failed:" << username;
#endif
            return SharedNodePointer();
        }
    }

    userPerms = setPermissionsForUser(isLocalUser, verifiedUsername, nodeConnection.senderSockAddr.getAddress(),
                                      nodeConnection.hardwareAddress);

    if (!userPerms.can(NodePermissions::Permission::canConnectToDomain)) {
        sendConnectionDeniedPacket("You lack the required permissions to connect to this domain.",
                nodeConnection.senderSockAddr, DomainHandler::ConnectionRefusedReason::NotAuthorized);
#ifdef WANT_DEBUG
        qDebug() << "stalling login due to permissions:" << username;
#endif
        return SharedNodePointer();
    }

    if (!userPerms.can(NodePermissions::Permission::canConnectPastMaxCapacity) && !isWithinMaxCapacity()) {
        // we can't allow this user to connect because we are at max capacity
        QString redirectOnMaxCapacity;
        const QVariant* redirectOnMaxCapacityVariant =
            valueForKeyPath(_server->_settingsManager.getSettingsMap(), MAXIMUM_USER_CAPACITY_REDIRECT_LOCATION);
        if (redirectOnMaxCapacityVariant && redirectOnMaxCapacityVariant->canConvert<QString>()) {
            redirectOnMaxCapacity = redirectOnMaxCapacityVariant->toString();
            qDebug() << "Redirection domain:" << redirectOnMaxCapacity;
        }

        sendConnectionDeniedPacket("Too many connected users.", nodeConnection.senderSockAddr,
                DomainHandler::ConnectionRefusedReason::TooManyUsers, redirectOnMaxCapacity);
#ifdef WANT_DEBUG
        qDebug() << "stalling login due to max capacity:" << username;
#endif
        return SharedNodePointer();
    }

    QUuid hintNodeID;

    // in case this is a node that's failing to connect
    // double check we don't have a node whose sockets match exactly already in the list
    limitedNodeList->eachNodeBreakable([&nodeConnection, &hintNodeID](const SharedNodePointer& node){
        if (node->getPublicSocket() == nodeConnection.publicSockAddr
            && node->getLocalSocket() == nodeConnection.localSockAddr) {
            // we have a node that already has these exact sockets - this occurs if a node
            // is unable to connect to the domain
            hintNodeID = node->getUUID();
            return false;
        }
        return true;
    });

    // add the connecting node (or re-use the matched one from eachNodeBreakable above)
    SharedNodePointer newNode = addVerifiedNodeFromConnectRequest(nodeConnection, hintNodeID);

    // set the edit rights for this user
    newNode->setPermissions(userPerms);

    // grab the linked data for our new node so we can set the username
    DomainServerNodeData* nodeData = reinterpret_cast<DomainServerNodeData*>(newNode->getLinkedData());

    // if we have a username from the connect request, set it on the DomainServerNodeData
    nodeData->setUsername(username);

    // set the hardware address passed in the connect request
    nodeData->setHardwareAddress(nodeConnection.hardwareAddress);

    // also add an interpolation to DomainServerNodeData so that servers can get username in stats
    nodeData->addOverrideForKey(USERNAME_UUID_REPLACEMENT_STATS_KEY,
                                uuidStringWithoutCurlyBraces(newNode->getUUID()), username);

#ifdef WANT_DEBUG
    qDebug() << "accepting login:" << username;
#endif

    return newNode;
}

SharedNodePointer DomainGatekeeper::addVerifiedNodeFromConnectRequest(const NodeConnectionData& nodeConnection,
                                                                      QUuid nodeID) {
    HifiSockAddr discoveredSocket = nodeConnection.senderSockAddr;
    SharedNetworkPeer connectedPeer = _icePeers.value(nodeConnection.connectUUID);

    if (connectedPeer) {
        //  this user negotiated a connection with us via ICE, so re-use their ICE client ID
        nodeID = nodeConnection.connectUUID;

        if (connectedPeer->getActiveSocket()) {
            // set their discovered socket to whatever the activated socket on the network peer object was
            discoveredSocket = *connectedPeer->getActiveSocket();
        }
    } else {
        // we got a connectUUID we didn't recognize, either use the hinted node ID or randomly generate a new one
        if (nodeID.isNull()) {
            nodeID = QUuid::createUuid();
        }
    }

    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();

    SharedNodePointer newNode = limitedNodeList->addOrUpdateNode(nodeID, nodeConnection.nodeType,
                                                                 nodeConnection.publicSockAddr, nodeConnection.localSockAddr);

    // So that we can send messages to this node at will - we need to activate the correct socket on this node now
    newNode->activateMatchingOrNewSymmetricSocket(discoveredSocket);

    return newNode;
}

bool DomainGatekeeper::verifyUserSignature(const QString& username,
                                           const QByteArray& usernameSignature,
                                           const HifiSockAddr& senderSockAddr) {
    // it's possible this user can be allowed to connect, but we need to check their username signature
    auto lowerUsername = username.toLower();
    QByteArray publicKeyArray = _userPublicKeys.value(lowerUsername);

    const QUuid& connectionToken = _connectionTokenHash.value(lowerUsername);

    if (!publicKeyArray.isEmpty() && !connectionToken.isNull()) {
        // if we do have a public key for the user, check for a signature match

        const unsigned char* publicKeyData = reinterpret_cast<const unsigned char*>(publicKeyArray.constData());

        // first load up the public key into an RSA struct
        RSA* rsaPublicKey = d2i_RSA_PUBKEY(NULL, &publicKeyData, publicKeyArray.size());

        QByteArray lowercaseUsernameUTF8 = lowerUsername.toUtf8();
        QByteArray usernameWithToken = QCryptographicHash::hash(lowercaseUsernameUTF8.append(connectionToken.toRfc4122()),
                                                                QCryptographicHash::Sha256);

        if (rsaPublicKey) {
            int decryptResult = RSA_verify(NID_sha256,
                                           reinterpret_cast<const unsigned char*>(usernameWithToken.constData()),
                                           usernameWithToken.size(),
                                           reinterpret_cast<const unsigned char*>(usernameSignature.constData()),
                                           usernameSignature.size(),
                                           rsaPublicKey);

            if (decryptResult == 1) {
                qDebug() << "Username signature matches for" << username;

                // free up the public key and remove connection token before we return
                RSA_free(rsaPublicKey);
                _connectionTokenHash.remove(username);

                return true;

            } else {
                if (!senderSockAddr.isNull()) {
                    qDebug() << "Error decrypting username signature for " << username << "- denying connection.";
                    sendConnectionDeniedPacket("Error decrypting username signature.", senderSockAddr,
                        DomainHandler::ConnectionRefusedReason::LoginError);
                }

                // free up the public key, we don't need it anymore
                RSA_free(rsaPublicKey);
            }

        } else {

            // we can't let this user in since we couldn't convert their public key to an RSA key we could use
            if (!senderSockAddr.isNull()) {
                qDebug() << "Couldn't convert data to RSA key for" << username << "- denying connection.";
                sendConnectionDeniedPacket("Couldn't convert data to RSA key.", senderSockAddr,
                    DomainHandler::ConnectionRefusedReason::LoginError);
            }
        }
    } else {
        if (!senderSockAddr.isNull()) {
            qDebug() << "Insufficient data to decrypt username signature - delaying connection.";
        }
    }

    requestUserPublicKey(username); // no joy.  maybe next time?
    return false;
}

bool DomainGatekeeper::isWithinMaxCapacity() {
    // find out what our maximum capacity is
    const QVariant* maximumUserCapacityVariant =
        valueForKeyPath(_server->_settingsManager.getSettingsMap(), MAXIMUM_USER_CAPACITY);
    unsigned int maximumUserCapacity = maximumUserCapacityVariant ? maximumUserCapacityVariant->toUInt() : 0;

    if (maximumUserCapacity > 0) {
        unsigned int connectedUsers = _server->countConnectedUsers();

        if (connectedUsers >= maximumUserCapacity) {
            qDebug() << connectedUsers << "/" << maximumUserCapacity << "users connected, denying new connection.";
            return false;
        }

        qDebug() << connectedUsers << "/" << maximumUserCapacity << "users connected, allowing new connection.";
    }

    return true;
}


void DomainGatekeeper::preloadAllowedUserPublicKeys() {
    QStringList allowedUsers = _server->_settingsManager.getAllNames();

    if (allowedUsers.size() > 0) {
        // in the future we may need to limit how many requests here - for now assume that lists of allowed users are not
        // going to create > 100 requests
        foreach(const QString& username, allowedUsers) {
            requestUserPublicKey(username);
        }
    }
}

void DomainGatekeeper::requestUserPublicKey(const QString& username) {
    // don't request public keys for the standard psuedo-account-names
    if (NodePermissions::standardNames.contains(username, Qt::CaseInsensitive)) {
        return;
    }

    QString lowerUsername = username.toLower();
    if (_inFlightPublicKeyRequests.contains(lowerUsername)) {
        // public-key request for this username is already flight, not rerequesting
        return;
    }
    _inFlightPublicKeyRequests += lowerUsername;

    // even if we have a public key for them right now, request a new one in case it has just changed
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "publicKeyJSONCallback";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "publicKeyJSONErrorCallback";


    const QString USER_PUBLIC_KEY_PATH = "api/v1/users/%1/public_key";

    qDebug() << "Requesting public key for user" << username;

    DependencyManager::get<AccountManager>()->sendRequest(USER_PUBLIC_KEY_PATH.arg(username),
                                              AccountManagerAuth::None,
                                              QNetworkAccessManager::GetOperation, callbackParams);
}

QString extractUsernameFromPublicKeyRequest(QNetworkReply& requestReply) {
    // extract the username from the request url
    QString username;
    const QString PUBLIC_KEY_URL_REGEX_STRING = "api\\/v1\\/users\\/([A-Za-z0-9_\\.]+)\\/public_key";
    QRegExp usernameRegex(PUBLIC_KEY_URL_REGEX_STRING);
    if (usernameRegex.indexIn(requestReply.url().toString()) != -1) {
        username = usernameRegex.cap(1);
    }
    return username.toLower();
}

void DomainGatekeeper::publicKeyJSONCallback(QNetworkReply& requestReply) {
    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    QString username = extractUsernameFromPublicKeyRequest(requestReply);

    if (jsonObject["status"].toString() == "success" && !username.isEmpty()) {
        // pull the public key as a QByteArray from this response
        const QString JSON_DATA_KEY = "data";
        const QString JSON_PUBLIC_KEY_KEY = "public_key";

        _userPublicKeys[username.toLower()] =
            QByteArray::fromBase64(jsonObject[JSON_DATA_KEY].toObject()[JSON_PUBLIC_KEY_KEY].toString().toUtf8());
    }

    _inFlightPublicKeyRequests.remove(username);
}

void DomainGatekeeper::publicKeyJSONErrorCallback(QNetworkReply& requestReply) {
    qDebug() << "publicKey api call failed:" << requestReply.error();
    QString username = extractUsernameFromPublicKeyRequest(requestReply);
    _inFlightPublicKeyRequests.remove(username);
}

void DomainGatekeeper::sendProtocolMismatchConnectionDenial(const HifiSockAddr& senderSockAddr) {
    QString protocolVersionError = "Protocol version mismatch - Domain version: " + QCoreApplication::applicationVersion();

    qDebug() << "Protocol Version mismatch - denying connection.";

    sendConnectionDeniedPacket(protocolVersionError, senderSockAddr,
                               DomainHandler::ConnectionRefusedReason::ProtocolMismatch);
}

void DomainGatekeeper::sendConnectionDeniedPacket(const QString& reason, const HifiSockAddr& senderSockAddr,
                                                  DomainHandler::ConnectionRefusedReason reasonCode,
                                                  QString extraInfo) {
    // this is an agent and we've decided we won't let them connect - send them a packet to deny connection
    QByteArray utfReasonString = reason.toUtf8();
    quint16 reasonSize = utfReasonString.size();

    QByteArray utfExtraInfo = extraInfo.toUtf8();
    quint16 extraInfoSize = utfExtraInfo.size();

    // setup the DomainConnectionDenied packet
    auto connectionDeniedPacket = NLPacket::create(PacketType::DomainConnectionDenied,
                                            sizeof(uint8_t) + // reasonCode
                                            reasonSize + sizeof(reasonSize) +
                                            extraInfoSize + sizeof(extraInfoSize));

    // pack in the reason the connection was denied (the client displays this)
    uint8_t reasonCodeWire = (uint8_t)reasonCode;
    connectionDeniedPacket->writePrimitive(reasonCodeWire);
    connectionDeniedPacket->writePrimitive(reasonSize);
    connectionDeniedPacket->write(utfReasonString);

    // write the extra info as well
    connectionDeniedPacket->writePrimitive(extraInfoSize);
    connectionDeniedPacket->write(utfExtraInfo);

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

void DomainGatekeeper::processICEPeerInformationPacket(QSharedPointer<ReceivedMessage> message) {
    // loop through the packet and pull out network peers
    // any peer we don't have we add to the hash, otherwise we update
    QDataStream iceResponseStream(message->getMessage());

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

void DomainGatekeeper::processICEPingPacket(QSharedPointer<ReceivedMessage> message) {
    auto limitedNodeList = DependencyManager::get<LimitedNodeList>();
    auto pingReplyPacket = limitedNodeList->constructICEPingReplyPacket(*message, limitedNodeList->getSessionUUID());

    limitedNodeList->sendPacket(std::move(pingReplyPacket), message->getSenderSockAddr());
}

void DomainGatekeeper::processICEPingReplyPacket(QSharedPointer<ReceivedMessage> message) {
    QDataStream packetStream(message->getMessage());

    QUuid nodeUUID;
    packetStream >> nodeUUID;

    SharedNetworkPeer sendingPeer = _icePeers.value(nodeUUID);

    if (sendingPeer) {
        // we had this NetworkPeer in our connecting list - add the right sock addr to our connected list
        sendingPeer->activateMatchingOrNewSymmetricSocket(message->getSenderSockAddr());
    }
}

void DomainGatekeeper::getGroupMemberships(const QString& username) {
    // loop through the groups mentioned on the settings page and ask if this user is in each.  The replies
    // will be received asynchronously and permissions will be updated as the answers come in.

    QJsonObject json;
    QSet<QString> groupIDSet;
    foreach (QUuid groupID, _server->_settingsManager.getGroupIDs() + _server->_settingsManager.getBlacklistGroupIDs()) {
        groupIDSet += groupID.toString().mid(1,36);
    }

    if (groupIDSet.isEmpty()) {
        // if no groups are in the permissions settings, don't ask who is in which groups.
        return;
    }

    QJsonArray groupIDs = QJsonArray::fromStringList(groupIDSet.toList());
    json["groups"] = groupIDs;


    // if we've already asked, wait for the answer before asking again
    QString lowerUsername = username.toLower();
    if (_inFlightGroupMembershipsRequests.contains(lowerUsername)) {
        // public-key request for this username is already flight, not rerequesting
        return;
    }
    _inFlightGroupMembershipsRequests += lowerUsername;


    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "getIsGroupMemberJSONCallback";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "getIsGroupMemberErrorCallback";

    const QString GET_IS_GROUP_MEMBER_PATH = "api/v1/groups/members/%2";
    DependencyManager::get<AccountManager>()->sendRequest(GET_IS_GROUP_MEMBER_PATH.arg(username),
                                                          AccountManagerAuth::Required,
                                                          QNetworkAccessManager::PostOperation, callbackParams,
                                                          QJsonDocument(json).toJson());

}

QString extractUsernameFromGroupMembershipsReply(QNetworkReply& requestReply) {
    // extract the username from the request url
    QString username;
    const QString GROUP_MEMBERSHIPS_URL_REGEX_STRING = "api\\/v1\\/groups\\/members\\/([A-Za-z0-9_\\.]+)";
    QRegExp usernameRegex(GROUP_MEMBERSHIPS_URL_REGEX_STRING);
    if (usernameRegex.indexIn(requestReply.url().toString()) != -1) {
        username = usernameRegex.cap(1);
    }
    return username.toLower();
}

void DomainGatekeeper::getIsGroupMemberJSONCallback(QNetworkReply& requestReply) {
    // {
    //     "data":{
    //         "username":"sethalves",
    //         "groups":{
    //             "fd55479a-265d-4990-854e-3d04214ad1b0":{
    //                 "name":"Blerg Blah",
    //                 "rank":{
    //                     "name":"admin",
    //                     "order":1
    //                 }
    //             }
    //         }
    //     },
    //     "status":"success"
    // }

    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    if (jsonObject["status"].toString() == "success") {
        QJsonObject data = jsonObject["data"].toObject();
        QJsonObject groups = data["groups"].toObject();
        QString username = data["username"].toString();
        _server->_settingsManager.clearGroupMemberships(username);
        foreach (auto groupID, groups.keys()) {
            QJsonObject group = groups[groupID].toObject();
            QJsonObject rank = group["rank"].toObject();
            QUuid rankID = QUuid(rank["id"].toString());
            _server->_settingsManager.recordGroupMembership(username, groupID, rankID);
        }
    } else {
        qDebug() << "getIsGroupMember api call returned:" << QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }

    _inFlightGroupMembershipsRequests.remove(extractUsernameFromGroupMembershipsReply(requestReply));
}

void DomainGatekeeper::getIsGroupMemberErrorCallback(QNetworkReply& requestReply) {
    qDebug() << "getIsGroupMember api call failed:" << requestReply.error();
    _inFlightGroupMembershipsRequests.remove(extractUsernameFromGroupMembershipsReply(requestReply));
}

void DomainGatekeeper::getDomainOwnerFriendsList() {
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "getDomainOwnerFriendsListJSONCallback";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "getDomainOwnerFriendsListErrorCallback";

    const QString GET_FRIENDS_LIST_PATH = "api/v1/user/friends";
    DependencyManager::get<AccountManager>()->sendRequest(GET_FRIENDS_LIST_PATH, AccountManagerAuth::Required,
                                                          QNetworkAccessManager::GetOperation, callbackParams, QByteArray(),
                                                          NULL, QVariantMap());
}

void DomainGatekeeper::getDomainOwnerFriendsListJSONCallback(QNetworkReply& requestReply) {
    // {
    //     status: "success",
    //     data: {
    //         friends: [
    //             "chris",
    //             "freidrica",
    //             "G",
    //             "huffman",
    //             "leo",
    //             "philip",
    //             "ryan",
    //             "sam",
    //             "ZappoMan"
    //         ]
    //     }
    // }
    QJsonObject jsonObject = QJsonDocument::fromJson(requestReply.readAll()).object();
    if (jsonObject["status"].toString() == "success") {
        _domainOwnerFriends.clear();
        QJsonArray friends = jsonObject["data"].toObject()["friends"].toArray();
        for (int i = 0; i < friends.size(); i++) {
            _domainOwnerFriends += friends.at(i).toString();
        }
    } else {
        qDebug() << "getDomainOwnerFriendsList api call returned:" << QJsonDocument(jsonObject).toJson(QJsonDocument::Compact);
    }
}

void DomainGatekeeper::getDomainOwnerFriendsListErrorCallback(QNetworkReply& requestReply) {
    qDebug() << "getDomainOwnerFriendsList api call failed:" << requestReply.error();
}

void DomainGatekeeper::refreshGroupsCache() {
    // if agents are connected to this domain, refresh our cached information about groups and memberships in such.
    getDomainOwnerFriendsList();

    auto nodeList = DependencyManager::get<LimitedNodeList>();
    nodeList->eachNode([&](const SharedNodePointer& node) {
        if (!node->getPermissions().isAssignment) {
            // this node is an agent
            const QString& verifiedUserName = node->getPermissions().getVerifiedUserName();
            if (!verifiedUserName.isEmpty()) {
                getGroupMemberships(verifiedUserName);
            }
        }
    });

    _server->_settingsManager.apiRefreshGroupInformation();

    updateNodePermissions();

#if WANT_DEBUG
    _server->_settingsManager.debugDumpGroupsState();
#endif
}
