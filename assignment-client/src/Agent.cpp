//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtScript/QScriptEngine>
#include <QtNetwork/QtNetwork>

#include <AvatarData.h>
#include <NodeList.h>
#include <VoxelConstants.h>

#include "Agent.h"
#include "voxels/VoxelScriptingInterface.h"

Agent::Agent(const unsigned char* dataBuffer, int numBytes) : Assignment(dataBuffer, numBytes) {
    
}

void Agent::run() {
    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AGENT);
    nodeList->setNodeTypesOfInterest(&NODE_TYPE_VOXEL_SERVER, 1);

    nodeList->getNodeSocket()->setBlocking(false);
    
    QNetworkAccessManager manager;
    
    // figure out the URL for the script for this agent assignment
    QString scriptURLString("http://%1:8080/assignment/%2");
    scriptURLString = scriptURLString.arg(NodeList::getInstance()->getDomainIP().toString(),
                                         this->getUUIDStringWithoutCurlyBraces());
    QUrl scriptURL(scriptURLString);
    
    qDebug() << "Attemping download of " << scriptURL << "\n";
    
    QNetworkReply* reply = manager.get(QNetworkRequest(scriptURL));
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    
    QString scriptString = QString(reply->readAll());
    
    QScriptEngine engine;
    
    QScriptValue agentValue = engine.newQObject(this);
    engine.globalObject().setProperty("Agent", agentValue);
    
    VoxelScriptingInterface voxelScripter;
    QScriptValue voxelScripterValue =  engine.newQObject(&voxelScripter);
    engine.globalObject().setProperty("Voxels", voxelScripterValue);
    
    QScriptValue treeScaleValue = engine.newVariant(QVariant(TREE_SCALE));
    engine.globalObject().setProperty("TREE_SCALE", treeScaleValue);
    
    const long long VISUAL_DATA_SEND_INTERVAL_USECS = (1 / 60.0f) * 1000 * 1000;
    
    QScriptValue visualSendIntervalValue = engine.newVariant((QVariant(VISUAL_DATA_SEND_INTERVAL_USECS / 1000)));
    engine.globalObject().setProperty("VISUAL_DATA_SEND_INTERVAL_MS", visualSendIntervalValue);
    
    qDebug() << "Downloaded script:" << scriptString << "\n";
    QScriptValue result = engine.evaluate(scriptString);
    qDebug() << "Evaluated script.\n";
    
    if (engine.hasUncaughtException()) {
        int line = engine.uncaughtExceptionLineNumber();
        qDebug() << "Uncaught exception at line" << line << ":" << result.toString() << "\n";
    }
    
    timeval thisSend;
    timeval lastDomainServerCheckIn = {};
    int numMicrosecondsSleep = 0;
    
    sockaddr_in senderAddress;
    unsigned char receivedData[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
    bool hasVoxelServer = false;
    
    while (!_shouldStop) {
        // update the thisSend timeval to the current time
        gettimeofday(&thisSend, NULL);
        
        // if we're not hearing from the domain-server we should stop running
        if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            break;
        }
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        if (!hasVoxelServer) {
            for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                if (node->getType() == NODE_TYPE_VOXEL_SERVER) {
                    hasVoxelServer = true;
                }
            }
        } else {
            // allow the scripter's call back to setup visual data
            emit willSendVisualDataCallback();
            
            if (engine.hasUncaughtException()) {
                int line = engine.uncaughtExceptionLineNumber();
                qDebug() << "Uncaught exception at line" << line << ":" << engine.uncaughtException().toString() << "\n";
            }
            
            voxelScripter.getVoxelPacketSender()->processWithoutSleep();
        }
        
        while (NodeList::getInstance()->getNodeSocket()->receive((sockaddr*) &senderAddress, receivedData, &receivedBytes)) {
            NodeList::getInstance()->processNodeData((sockaddr*) &senderAddress, receivedData, receivedBytes);
        }
        
        // sleep for the correct amount of time to have data send be consistently timed
        if ((numMicrosecondsSleep = VISUAL_DATA_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&thisSend))) > 0) {
            usleep(numMicrosecondsSleep);
        }
    }
    
}
