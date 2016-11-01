//
//  IceServer.cpp
//  ice-server/src
//
//  Created by Stephen Birarda on 2014-10-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "IceServer.h"

#include <openssl/x509.h>

#include <QtCore/QJsonDocument>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <LimitedNodeList.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>

const int CLEAR_INACTIVE_PEERS_INTERVAL_MSECS = 1 * 1000;
const int PEER_SILENCE_THRESHOLD_MSECS = 5 * 1000;

IceServer::IceServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _id(QUuid::createUuid()),
    _serverSocket(0, false),
    _activePeers()
{
    // start the ice-server socket
    qDebug() << "ice-server socket is listening on" << ICE_SERVER_DEFAULT_PORT;
    _serverSocket.bind(QHostAddress::AnyIPv4, ICE_SERVER_DEFAULT_PORT);

    // set processPacket as the verified packet callback for the udt::Socket
    _serverSocket.setPacketHandler([this](std::unique_ptr<udt::Packet> packet) { processPacket(std::move(packet));  });
    
    // set packetVersionMatch as the verify packet operator for the udt::Socket
    using std::placeholders::_1;
    _serverSocket.setPacketFilterOperator(std::bind(&IceServer::packetVersionMatch, this, _1));

    // setup our timer to clear inactive peers
    QTimer* inactivePeerTimer = new QTimer(this);
    connect(inactivePeerTimer, &QTimer::timeout, this, &IceServer::clearInactivePeers);
    inactivePeerTimer->start(CLEAR_INACTIVE_PEERS_INTERVAL_MSECS);

    // handle public keys when they arrive from the QNetworkAccessManager
    auto& networkAccessManager = NetworkAccessManager::getInstance();
    connect(&networkAccessManager, &QNetworkAccessManager::finished, this, &IceServer::publicKeyReplyFinished);
}

bool IceServer::packetVersionMatch(const udt::Packet& packet) {
    PacketType headerType = NLPacket::typeInHeader(packet);
    PacketVersion headerVersion = NLPacket::versionInHeader(packet);
    
    if (headerVersion == versionForPacketType(headerType)) {
        return true;
    } else {
        return false;
    }
}

void IceServer::processPacket(std::unique_ptr<udt::Packet> packet) {

    auto nlPacket = NLPacket::fromBase(std::move(packet));
    
    // make sure that this packet at least looks like something we can read
    if (nlPacket->getPayloadSize() >= NLPacket::localHeaderSize(PacketType::ICEServerHeartbeat)) {
        
        if (nlPacket->getType() == PacketType::ICEServerHeartbeat) {
            SharedNetworkPeer peer = addOrUpdateHeartbeatingPeer(*nlPacket);
            if (peer) {
                // so that we can send packets to the heartbeating peer when we need, we need to activate a socket now
                peer->activateMatchingOrNewSymmetricSocket(nlPacket->getSenderSockAddr());

                // we have an active and verified heartbeating peer
                // send them an ACK packet so they know that they are being heard and ready for ICE
                static auto ackPacket = NLPacket::create(PacketType::ICEServerHeartbeatACK);
                _serverSocket.writePacket(*ackPacket, nlPacket->getSenderSockAddr());
            } else {
                // we couldn't verify this peer - respond back to them so they know they may need to perform keypair re-generation
                static auto deniedPacket = NLPacket::create(PacketType::ICEServerHeartbeatDenied);
                _serverSocket.writePacket(*deniedPacket, nlPacket->getSenderSockAddr());
            }
        } else if (nlPacket->getType() == PacketType::ICEServerQuery) {
            QDataStream heartbeatStream(nlPacket.get());
            
            // this is a node hoping to connect to a heartbeating peer - do we have the heartbeating peer?
            QUuid senderUUID;
            heartbeatStream >> senderUUID;
            
            // pull the public and private sock addrs for this peer
            HifiSockAddr publicSocket, localSocket;
            heartbeatStream >> publicSocket >> localSocket;
            
            // check if this node also included a UUID that they would like to connect to
            QUuid connectRequestID;
            heartbeatStream >> connectRequestID;
            
            SharedNetworkPeer matchingPeer = _activePeers.value(connectRequestID);
            
            if (matchingPeer) {
                
                qDebug() << "Sending information for peer" << connectRequestID << "to peer" << senderUUID;
                
                // we have the peer they want to connect to - send them pack the information for that peer
                sendPeerInformationPacket(*matchingPeer, &nlPacket->getSenderSockAddr());
                
                // we also need to send them to the active peer they are hoping to connect to
                // create a dummy peer object we can pass to sendPeerInformationPacket
                
                NetworkPeer dummyPeer(senderUUID, publicSocket, localSocket);
                sendPeerInformationPacket(dummyPeer, matchingPeer->getActiveSocket());
            } else {
                qDebug() << "Peer" << senderUUID << "asked for" << connectRequestID << "but no matching peer found";
            }
        }
    }
}

SharedNetworkPeer IceServer::addOrUpdateHeartbeatingPeer(NLPacket& packet) {

    // pull the UUID, public and private sock addrs for this peer
    QUuid senderUUID;
    HifiSockAddr publicSocket, localSocket;
    QByteArray signature;

    QDataStream heartbeatStream(&packet);
    heartbeatStream >> senderUUID >> publicSocket >> localSocket;

    auto signedPlaintext = QByteArray::fromRawData(packet.getPayload(), heartbeatStream.device()->pos());
    heartbeatStream >> signature;

    // make sure this is a verified heartbeat before performing any more processing
    if (isVerifiedHeartbeat(senderUUID, signedPlaintext, signature)) {
        // make sure we have this sender in our peer hash
        SharedNetworkPeer matchingPeer = _activePeers.value(senderUUID);

        if (!matchingPeer) {
            // if we don't have this sender we need to create them now
            matchingPeer = QSharedPointer<NetworkPeer>::create(senderUUID, publicSocket, localSocket);
            _activePeers.insert(senderUUID, matchingPeer);

            qDebug() << "Added a new network peer" << *matchingPeer;
        } else {
            // we already had the peer so just potentially update their sockets
            matchingPeer->setPublicSocket(publicSocket);
            matchingPeer->setLocalSocket(localSocket);
        }

        // update our last heard microstamp for this network peer to now
        matchingPeer->setLastHeardMicrostamp(usecTimestampNow());
        
        return matchingPeer;
    } else {
        // not verified, return the empty peer object
        return SharedNetworkPeer();
    }
}

bool IceServer::isVerifiedHeartbeat(const QUuid& domainID, const QByteArray& plaintext, const QByteArray& signature) {
    // make sure we're not already waiting for a public key for this domain-server
    if (!_pendingPublicKeyRequests.contains(domainID)) {
        // check if we have a public key for this domain ID - if we do not then fire off the request for it
        auto it = _domainPublicKeys.find(domainID);
        if (it != _domainPublicKeys.end()) {

            // attempt to verify the signature for this heartbeat
            const auto rsaPublicKey = it->second.get();

            if (rsaPublicKey) {
                auto hashedPlaintext = QCryptographicHash::hash(plaintext, QCryptographicHash::Sha256);
                int verificationResult = RSA_verify(NID_sha256,
                                                    reinterpret_cast<const unsigned char*>(hashedPlaintext.constData()),
                                                    hashedPlaintext.size(),
                                                    reinterpret_cast<const unsigned char*>(signature.constData()),
                                                    signature.size(),
                                                    rsaPublicKey);

                if (verificationResult == 1) {
                    // this is the only success case - we return true here to indicate that the heartbeat is verified
                    return true;
                } else {
                    qDebug() << "Failed to verify heartbeat for" << domainID << "- re-requesting public key from API.";
                }

            } else {
                // we can't let this user in since we couldn't convert their public key to an RSA key we could use
                qWarning() << "Public key for" << domainID << "is not a usable RSA* public key.";
                qWarning() << "Re-requesting public key from API";
            }
        }

        // we could not verify this heartbeat (missing public key, could not load public key, bad actor)
        // ask the metaverse API for the right public key and return false to indicate that this is not verified
        requestDomainPublicKey(domainID);
    }

    return false;
}

void IceServer::requestDomainPublicKey(const QUuid& domainID) {
    // send a request to the metaverse API for the public key for this domain
    auto& networkAccessManager = NetworkAccessManager::getInstance();

    QUrl publicKeyURL { NetworkingConstants::METAVERSE_SERVER_URL };
    QString publicKeyPath = QString("/api/v1/domains/%1/public_key").arg(uuidStringWithoutCurlyBraces(domainID));
    publicKeyURL.setPath(publicKeyPath);

    QNetworkRequest publicKeyRequest { publicKeyURL };
    publicKeyRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    publicKeyRequest.setAttribute(QNetworkRequest::User, domainID);

    qDebug() << "Requesting public key for domain with ID" << domainID;

    // add this to the set of pending public key requests
    _pendingPublicKeyRequests.insert(domainID);

    networkAccessManager.get(publicKeyRequest);
}

void IceServer::publicKeyReplyFinished(QNetworkReply* reply) {
    // get the domain ID from the QNetworkReply attribute
    QUuid domainID = reply->request().attribute(QNetworkRequest::User).toUuid();

    if (reply->error() == QNetworkReply::NoError) {
        // pull out the public key and store it for this domain

        // the response should be JSON
        QJsonDocument responseDocument = QJsonDocument::fromJson(reply->readAll());

        static const QString DATA_KEY = "data";
        static const QString PUBLIC_KEY_KEY = "public_key";
        static const QString STATUS_KEY = "status";
        static const QString SUCCESS_VALUE = "success";

        auto responseObject = responseDocument.object();
        if (responseObject[STATUS_KEY].toString() == SUCCESS_VALUE) {
            auto dataObject = responseObject[DATA_KEY].toObject();
            if (dataObject.contains(PUBLIC_KEY_KEY)) {

                // grab the base 64 public key from the API response
                auto apiPublicKey = QByteArray::fromBase64(dataObject[PUBLIC_KEY_KEY].toString().toUtf8());

                // convert the downloaded public key to an RSA struct, if possible
                const unsigned char* publicKeyData = reinterpret_cast<const unsigned char*>(apiPublicKey.constData());

                RSA* rsaPublicKey = d2i_RSA_PUBKEY(NULL, &publicKeyData, apiPublicKey.size());

                if (rsaPublicKey) {
                    _domainPublicKeys[domainID] = { rsaPublicKey, RSA_free };
                } else {
                    qWarning() << "Could not convert in-memory public key for" << domainID << "to usable RSA public key.";
                    qWarning() << "Public key will be re-requested on next heartbeat.";
                }

            } else {
                qWarning() << "There was no public key present in response for domain with ID" << domainID;
            }
        } else {
            qWarning() << "The metaverse API did not return success for public key request for domain with ID" << domainID;
        }

    } else {
        // there was a problem getting the public key for the domain
        // log it since it will be re-requested on the next heartbeat

        qWarning() << "Error retreiving public key for domain with ID" << domainID << "-" <<  reply->errorString();
    }

    // remove this domain ID from the list of pending public key requests
    _pendingPublicKeyRequests.remove(domainID);

    reply->deleteLater();
}

void IceServer::sendPeerInformationPacket(const NetworkPeer& peer, const HifiSockAddr* destinationSockAddr) {
    auto peerPacket = NLPacket::create(PacketType::ICEServerPeerInformation);

    // get the byte array for this peer
    peerPacket->write(peer.toByteArray());
    
    // write the current packet
    _serverSocket.writePacket(*peerPacket, *destinationSockAddr);
}

void IceServer::clearInactivePeers() {
    NetworkPeerHash::iterator peerItem = _activePeers.begin();

    while (peerItem != _activePeers.end()) {
        SharedNetworkPeer peer = peerItem.value();

        if ((usecTimestampNow() - peer->getLastHeardMicrostamp()) > (PEER_SILENCE_THRESHOLD_MSECS * 1000)) {
            qDebug() << "Removing peer from memory for inactivity -" << *peer;

            // if we had a public key for this domain, remove it now
            _domainPublicKeys.erase(peer->getUUID());

            // remove the peer object
            peerItem = _activePeers.erase(peerItem);
        } else {
            // we didn't kill this peer, push the iterator forwards
            ++peerItem;
        }
    }
}
