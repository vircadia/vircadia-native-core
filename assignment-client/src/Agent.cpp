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
                _scriptEngine.getVoxelScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
                                                                                (unsigned char*) dataByteArray.data(),
                                                                                dataByteArray.size());
                break;
            case NODE_TYPE_PARTICLE_SERVER:
                _scriptEngine.getParticleScriptingInterface()->getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
                                                                                (unsigned char*) dataByteArray.data(),
                                                                                dataByteArray.size());
                break;
        }
    } else {
        NodeList::getInstance()->processNodeData(senderSockAddr, (unsigned char*) dataByteArray.data(), dataByteArray.size());
    }
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AGENT);
    
    // XXXBHG - this seems less than ideal. There might be classes (like jurisdiction listeners, that need access to
    // other node types, but for them to get access to those node types, we have to add them here. It seems like 
    // NodeList should support adding types of interest
    const NODE_TYPE AGENT_NODE_TYPES_OF_INTEREST[] = { NODE_TYPE_VOXEL_SERVER, NODE_TYPE_PARTICLE_SERVER  };
    
    nodeList->setNodeTypesOfInterest(AGENT_NODE_TYPES_OF_INTEREST, sizeof(AGENT_NODE_TYPES_OF_INTEREST));
    
    // figure out the URL for the script for this agent assignment
    QString scriptURLString("http://%1:8080/assignment/%2");
    scriptURLString = scriptURLString.arg(NodeList::getInstance()->getDomainIP().toString(),
                                          uuidStringWithoutCurlyBraces(_uuid));
    
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = networkManager->get(QNetworkRequest(QUrl(scriptURLString)));
    
    qDebug() << "Downloading script at" << scriptURLString << "\n";
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    
    loop.exec();
    
    QString scriptContents(reply->readAll());
    
    qDebug() << "Downloaded script:" << scriptContents << "\n";
    
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

    _scriptEngine.setScriptContents(scriptContents);
    _scriptEngine.run();    
}
