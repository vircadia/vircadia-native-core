//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QEventLoop>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <AvatarData.h>
#include <NodeList.h>
#include <UUID.h>
#include <VoxelConstants.h>

#include "Agent.h"
#include "voxels/VoxelScriptingInterface.h"

Agent::Agent(const unsigned char* dataBuffer, int numBytes) :
    Assignment(dataBuffer, numBytes),
    _shouldStop(false)
{
}


void Agent::stop() {
    _shouldStop = true;
}

QScriptValue vec3toScriptValue(QScriptEngine *engine, const glm::vec3 &vec3) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", vec3.x);
    obj.setProperty("y", vec3.y);
    obj.setProperty("z", vec3.z);
    return obj;
}

void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3) {
    vec3.x = object.property("x").toVariant().toFloat();
    vec3.y = object.property("y").toVariant().toFloat();
    vec3.z = object.property("z").toVariant().toFloat();
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AGENT);
    
    const char AGENT_NODE_TYPES_OF_INTEREST[1] = { NODE_TYPE_VOXEL_SERVER };
    
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
    qScriptRegisterMetaType(&engine, vec3toScriptValue, vec3FromScriptValue);
    
    QScriptValue agentValue = engine.newQObject(this);
    engine.globalObject().setProperty("Agent", agentValue);
    
    VoxelScriptingInterface voxelScripter;
    QScriptValue voxelScripterValue =  engine.newQObject(&voxelScripter);
    engine.globalObject().setProperty("Voxels", voxelScripterValue);
    
    QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
    engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);
    
    const unsigned int VISUAL_DATA_CALLBACK_USECS = (1.0 / 60.0) * 1000 * 1000;
    
    // let the VoxelPacketSender know how frequently we plan to call it
    voxelScripter.getVoxelPacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);
    
    qDebug() << "Downloaded script:" << scriptContents << "\n";
    QScriptValue result = engine.evaluate(scriptContents);
    qDebug() << "Evaluated script.\n";
    
    if (engine.hasUncaughtException()) {
        int line = engine.uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
    }
    
    timeval startTime;
    gettimeofday(&startTime, NULL);
    
    timeval lastDomainServerCheckIn = {};
    
    
    HifiSockAddr senderSockAddr;
    unsigned char receivedData[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
    int thisFrame = 0;
    
    NodeList::getInstance()->startSilentNodeRemovalThread();
    
    while (!_shouldStop) {
        
        // if we're not hearing from the domain-server we should stop running
        if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            break;
        }
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        int usecToSleep = usecTimestamp(&startTime) + (thisFrame++ * VISUAL_DATA_CALLBACK_USECS) - usecTimestampNow();
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        }
        
        if (voxelScripter.getVoxelPacketSender()->voxelServersExist()) {
            timeval thisSend = {};
            gettimeofday(&thisSend, NULL);
            // allow the scripter's call back to setup visual data
            emit willSendVisualDataCallback();
            
            // release the queue of edit voxel messages.
            voxelScripter.getVoxelPacketSender()->releaseQueuedMessages();
            
            // since we're in non-threaded mode, call process so that the packets are sent
            voxelScripter.getVoxelPacketSender()->process();
        }
        
        if (engine.hasUncaughtException()) {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << engine.uncaughtException().toString() << "\n";
        }
        
        while (nodeList->getNodeSocket().hasPendingDatagrams() &&
               (receivedBytes = nodeList->getNodeSocket().readDatagram((char*) receivedBytes,
                                                                       MAX_PACKET_SIZE,
                                                                       senderSockAddr.getAddressPointer(),
                                                                       senderSockAddr.getPortPointer()))
                && packetVersionMatch(receivedData)) {
            if (receivedData[0] == PACKET_TYPE_VOXEL_JURISDICTION) {
                voxelScripter.getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
                                                                             receivedData,
                                                                             receivedBytes);
            } else {
                NodeList::getInstance()->processNodeData(senderSockAddr, receivedData, receivedBytes);
            }
        }
    }
    
    NodeList::getInstance()->stopSilentNodeRemovalThread();
}
