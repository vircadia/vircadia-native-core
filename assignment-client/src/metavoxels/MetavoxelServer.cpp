//
//  MetavoxelServer.cpp
//  hifi
//
//  Created by Andrzej Kapolka on 12/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QTimer>

#include <Logging.h>
#include <PacketHeaders.h>

#include <MetavoxelUtil.h>

#include "MetavoxelServer.h"
#include "Session.h"

MetavoxelServer::MetavoxelServer(const unsigned char* dataBuffer, int numBytes) :
    ThreadedAssignment(dataBuffer, numBytes) {
}

void MetavoxelServer::removeSession(const QUuid& sessionId) {
    delete _sessions.take(sessionId);
}

void MetavoxelServer::run() {
    // change the logging target name while the metavoxel server is running
    Logging::setTargetName("metavoxel-server");
    
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_METAVOXEL_SERVER);
    
    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_USECS / 1000);
    
    QTimer* pingNodesTimer = new QTimer(this);
    connect(pingNodesTimer, SIGNAL(timeout()), nodeList, SLOT(pingInactiveNodes()));
    pingNodesTimer->start(PING_INACTIVE_NODE_INTERVAL_USECS / 1000);
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);
}
    
void MetavoxelServer::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    switch (dataByteArray.at(0)) {
        case PACKET_TYPE_METAVOXEL_DATA:
            processData(dataByteArray, senderSockAddr);
            break;
        
        default:
            NodeList::getInstance()->processNodeData(senderSockAddr, (unsigned char*)dataByteArray.data(), dataByteArray.size());
            break;
    }
}

void MetavoxelServer::processData(const QByteArray& data, const HifiSockAddr& sender) {
    // read the session id
    int headerPlusIDSize;
    QUuid sessionID = readSessionID(data, sender, headerPlusIDSize);
    if (sessionID.isNull()) {
        return;
    }
    
    // forward to session, creating if necessary
    Session*& session = _sessions[sessionID];
    if (!session) {
        session = new Session(this, sessionID, QByteArray::fromRawData(data.constData(), headerPlusIDSize));
    }
    session->receivedData(data, sender);
}
