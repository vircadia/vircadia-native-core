//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QCoreApplication>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <AvatarData.h>
#include <NodeList.h>
#include <PacketHeaders.h>
#include <UUID.h>
#include <VoxelConstants.h>
#include <ParticlesScriptingInterface.h>

#include "Agent.h"

Agent::Agent(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _voxelEditSender(),
    _particleEditSender()
{
    _scriptEngine.getVoxelsScriptingInterface()->setPacketSender(&_voxelEditSender);
    _scriptEngine.getParticlesScriptingInterface()->setPacketSender(&_particleEditSender);
}

void Agent::readPendingDatagrams() {
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    NodeList* nodeList = NodeList::getInstance();
    
    while (readAvailableDatagram(receivedPacket, senderSockAddr)) {
        if (nodeList->packetVersionAndHashMatch(receivedPacket)) {
            PacketType datagramPacketType = packetTypeForPacket(receivedPacket);
            if (datagramPacketType == PacketTypeJurisdiction) {
                int headerBytes = numBytesForPacketHeader(receivedPacket);
                
                SharedNodePointer matchedNode = nodeList->sendingNodeForPacket(receivedPacket);
                
                if (matchedNode) {
                    // PacketType_JURISDICTION, first byte is the node type...
                    switch (receivedPacket[headerBytes]) {
                        case NodeType::VoxelServer:
                            _scriptEngine.getVoxelsScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(matchedNode,
                                                                                                                        receivedPacket);
                            break;
                        case NodeType::ParticleServer:
                            _scriptEngine.getParticlesScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(matchedNode,
                                                                                                                           receivedPacket);
                            break;
                    }
                }
                
            } else if (datagramPacketType == PacketTypeParticleAddResponse) {
                // this will keep creatorTokenIDs to IDs mapped correctly
                Particle::handleAddParticleResponse(receivedPacket);
                
                // also give our local particle tree a chance to remap any internal locally created particles
                _particleTree.handleAddParticleResponse(receivedPacket);
            } else {
                NodeList::getInstance()->processNodeData(senderSockAddr, receivedPacket);
            }
        }
    }
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NodeType::Agent);
    
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer);
    
    // figure out the URL for the script for this agent assignment
    QString scriptURLString("http://%1:8080/assignment/%2");
    scriptURLString = scriptURLString.arg(NodeList::getInstance()->getDomainInfo().getIP().toString(),
                                          uuidStringWithoutCurlyBraces(_uuid));
    
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(scriptURLString)));
    
    qDebug() << "Downloading script at" << scriptURLString;
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    
    loop.exec();
    
    // let the AvatarData class use our QNetworkAcessManager
    AvatarData::setNetworkAccessManager(networkManager);
    
    QString scriptContents(reply->readAll());
    
    qDebug() << "Downloaded script:" << scriptContents;
    
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_USECS / 1000);
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);
    
    // tell our script engine about our local particle tree
    _scriptEngine.getParticlesScriptingInterface()->setParticleTree(&_particleTree);
    
    // setup an Avatar for the script to use
    AvatarData scriptedAvatar;
    
    // call model URL setters with empty URLs so our avatar, if user, will have the default models
    scriptedAvatar.setFaceModelURL(QUrl());
    scriptedAvatar.setSkeletonModelURL(QUrl());
    
    // give this AvatarData object to the script engine
    _scriptEngine.setAvatarData(&scriptedAvatar, "Avatar");
    
    // register ourselves to the script engine
    _scriptEngine.registerGlobalObject("Agent", this);

    _scriptEngine.setScriptContents(scriptContents);
    _scriptEngine.run();    
}
