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

void Agent::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    PacketType datagramPacketType = packetTypeForPacket(dataByteArray);
    if (datagramPacketType == PacketTypeJurisdiction) {
        int headerBytes = numBytesForPacketHeader(dataByteArray);
        
        QUuid nodeUUID;
        SharedNodePointer matchedNode = NodeList::getInstance()->nodeWithUUID(nodeUUID);
        
        if (matchedNode) {
            // PacketType_JURISDICTION, first byte is the node type...
            switch (dataByteArray[headerBytes]) {
                case NodeType::VoxelServer:
                    _scriptEngine.getVoxelsScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(matchedNode,
                                                                                                                dataByteArray);
                    break;
                case NodeType::ParticleServer:
                    _scriptEngine.getParticlesScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(matchedNode,
                                                                                                                   dataByteArray);
                    break;
            }
        }
        
    } else if (datagramPacketType == PacketTypeParticleAddResponse) {
        // this will keep creatorTokenIDs to IDs mapped correctly
        Particle::handleAddParticleResponse(dataByteArray);
        
        // also give our local particle tree a chance to remap any internal locally created particles
        _particleTree.handleAddParticleResponse(dataByteArray);
    } else {
        NodeList::getInstance()->processNodeData(senderSockAddr, dataByteArray);
    }
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NodeType::Agent);
    
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer);
    
    // figure out the URL for the script for this agent assignment
    QString scriptURLString("http://%1:8080/assignment/%2");
    scriptURLString = scriptURLString.arg(NodeList::getInstance()->getDomainIP().toString(),
                                          uuidStringWithoutCurlyBraces(_uuid));
    
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(scriptURLString)));
    
    qDebug() << "Downloading script at" << scriptURLString;
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    
    loop.exec();
    
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
    
    QTimer* pingNodesTimer = new QTimer(this);
    connect(pingNodesTimer, SIGNAL(timeout()), nodeList, SLOT(pingInactiveNodes()));
    pingNodesTimer->start(PING_INACTIVE_NODE_INTERVAL_USECS / 1000);
    
    // tell our script engine about our local particle tree
    _scriptEngine.getParticlesScriptingInterface()->setParticleTree(&_particleTree);
    
    // setup an Avatar for the script to use
    AvatarData scriptedAvatar;
    
    // give this AvatarData object to the script engine
    _scriptEngine.setAvatarData(&scriptedAvatar, "Avatar");
    
    // register ourselves to the script engine
    _scriptEngine.registerGlobalObject("Agent", this);

    _scriptEngine.setScriptContents(scriptContents);
    _scriptEngine.run();    
}
