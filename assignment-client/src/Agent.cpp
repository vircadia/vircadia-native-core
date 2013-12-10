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
                _voxelScriptingInterface.getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
                                                                                (unsigned char*) dataByteArray.data(),
                                                                                dataByteArray.size());
                break;
            case NODE_TYPE_PARTICLE_SERVER:
                _particleScriptingInterface.getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
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
    QScriptEngine engine;
    
    // register meta-type for glm::vec3 conversions
    registerMetaTypes(&engine);
    
    QScriptValue agentValue = engine.newQObject(this);
    engine.globalObject().setProperty("Agent", agentValue);
    
    QScriptValue voxelScripterValue =  engine.newQObject(&_voxelScriptingInterface);
    engine.globalObject().setProperty("Voxels", voxelScripterValue);

    QScriptValue particleScripterValue =  engine.newQObject(&_particleScriptingInterface);
    engine.globalObject().setProperty("Particles", particleScripterValue);
    
    QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
    engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);
    
    const unsigned int VISUAL_DATA_CALLBACK_USECS = (1.0 / 60.0) * 1000 * 1000;
    
    // let the VoxelPacketSender know how frequently we plan to call it
    _voxelScriptingInterface.getVoxelPacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);
    _particleScriptingInterface.getParticlePacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);
    
    qDebug() << "Downloaded script:" << scriptContents << "\n";
    QScriptValue result = engine.evaluate(scriptContents);
    qDebug() << "Evaluated script.\n";
    
    if (engine.hasUncaughtException()) {
        int line = engine.uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
    }
    
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    int thisFrame = 0;
    
    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_USECS / 1000);
    
    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);
    
    QTimer* pingNodesTimer = new QTimer(this);
    connect(pingNodesTimer, SIGNAL(timeout()), nodeList, SLOT(pingInactiveNodes()));
    pingNodesTimer->start(PING_INACTIVE_NODE_INTERVAL_USECS / 1000);
    
    while (!_isFinished) {
        
        int usecToSleep = usecTimestamp(&startTime) + (thisFrame++ * VISUAL_DATA_CALLBACK_USECS) - usecTimestampNow();
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
        
        QCoreApplication::processEvents();
        
        bool willSendVisualDataCallBack = false;
        
        if (_voxelScriptingInterface.getVoxelPacketSender()->voxelServersExist()) {            
            // allow the scripter's call back to setup visual data
            willSendVisualDataCallBack = true;
            
            // release the queue of edit voxel messages.
            _voxelScriptingInterface.getVoxelPacketSender()->releaseQueuedMessages();
            
            // since we're in non-threaded mode, call process so that the packets are sent
            _voxelScriptingInterface.getVoxelPacketSender()->process();
        }

        if (_particleScriptingInterface.getParticlePacketSender()->serversExist()) {
            // allow the scripter's call back to setup visual data
            willSendVisualDataCallBack = true;
            
            // release the queue of edit voxel messages.
            _particleScriptingInterface.getParticlePacketSender()->releaseQueuedMessages();
            
            // since we're in non-threaded mode, call process so that the packets are sent
            _particleScriptingInterface.getParticlePacketSender()->process();
        }
        
        if (willSendVisualDataCallBack) {
            emit willSendVisualDataCallback();
        }

        
        if (engine.hasUncaughtException()) {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << engine.uncaughtException().toString() << "\n";
        }
        
        
    }
}
