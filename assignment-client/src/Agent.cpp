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
#include <UUID.h>
#include <VoxelConstants.h>

#include "Agent.h"

Agent::Agent(const unsigned char* dataBuffer, int numBytes) :
    ThreadedAssignment(dataBuffer, numBytes)
{
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

void Agent::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    if (dataByteArray.data()[0] == PACKET_TYPE_VOXEL_JURISDICTION) {
        _voxelScriptingInterface.getJurisdictionListener()->queueReceivedPacket(senderSockAddr,
                                                                                (unsigned char*) dataByteArray.data(),
                                                                                dataByteArray.size());
    } else {
        NodeList::getInstance()->processNodeData(senderSockAddr, (unsigned char*) dataByteArray.data(), dataByteArray.size());
    }
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
    
    QScriptValue voxelScripterValue =  engine.newQObject(&_voxelScriptingInterface);
    engine.globalObject().setProperty("Voxels", voxelScripterValue);
    
    QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
    engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);
    
    const unsigned int VISUAL_DATA_CALLBACK_USECS = (1.0 / 60.0) * 1000 * 1000;
    
    // let the VoxelPacketSender know how frequently we plan to call it
    _voxelScriptingInterface.getVoxelPacketSender()->setProcessCallIntervalHint(VISUAL_DATA_CALLBACK_USECS);
    
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
        
        if (_voxelScriptingInterface.getVoxelPacketSender()->voxelServersExist()) {
            timeval thisSend = {};
            gettimeofday(&thisSend, NULL);
            // allow the scripter's call back to setup visual data
            emit willSendVisualDataCallback();
            
            // release the queue of edit voxel messages.
            _voxelScriptingInterface.getVoxelPacketSender()->releaseQueuedMessages();
            
            // since we're in non-threaded mode, call process so that the packets are sent
            _voxelScriptingInterface.getVoxelPacketSender()->process();
        }
        
        if (engine.hasUncaughtException()) {
            int line = engine.uncaughtExceptionLineNumber();
            qDebug() << "Uncaught exception at line" << line << ":" << engine.uncaughtException().toString() << "\n";
        }
        
        
    }
}
