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

Agent::Agent(const unsigned char* dataBuffer, int numBytes) :
    ThreadedAssignment(dataBuffer, numBytes)
{
}

void Agent::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    if (dataByteArray[0] == PACKET_TYPE_JURISDICTION) {
        int headerBytes = numBytesForPacketHeader((const unsigned char*) dataByteArray.constData());
        // PACKET_TYPE_JURISDICTION, first byte is the node type...
        switch (dataByteArray[headerBytes]) {
            case NODE_TYPE_VOXEL_SERVER:
                _scriptEngine.getVoxelsScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
                                                                                (unsigned char*) dataByteArray.data(),
                                                                                dataByteArray.size());
                break;
            case NODE_TYPE_PARTICLE_SERVER:
                _scriptEngine.getParticlesScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
                                                                                (unsigned char*) dataByteArray.data(),
                                                                                dataByteArray.size());
                break;
        }
    } else if (dataByteArray[0] == PACKET_TYPE_PARTICLE_ADD_RESPONSE) {
        // this will keep creatorTokenIDs to IDs mapped correctly
        Particle::handleAddParticleResponse((unsigned char*) dataByteArray.data(), dataByteArray.size());
    } else {
        NodeList::getInstance()->processNodeData(senderSockAddr, (unsigned char*) dataByteArray.data(), dataByteArray.size());
    }
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AGENT);
    
    nodeList->addSetOfNodeTypesToNodeInterestSet(QSet<NODE_TYPE>() << NODE_TYPE_AUDIO_MIXER << NODE_TYPE_AVATAR_MIXER);
    
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

    _scriptEngine.setScriptContents(scriptContents);
    _scriptEngine.run();    
}
