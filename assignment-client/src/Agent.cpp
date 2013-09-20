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

#include "Agent.h"
#include "voxels/VoxelScriptingInterface.h"

Agent::Agent(const unsigned char* dataBuffer, int numBytes) : Assignment(dataBuffer, numBytes) {
    
}

void Agent::run() {
    NodeList::getInstance()->setOwnerType(NODE_TYPE_AGENT);
    NodeList::getInstance()->setNodeTypesOfInterest(&NODE_TYPE_VOXEL_SERVER, 1);
    
    NodeList::getInstance()->getNodeSocket()->setBlocking(false);
    
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
    
    qDebug() << "Downloaded script:" << scriptString << "\n";
    qDebug() << "Evaluated script:" << engine.evaluate(scriptString).toString() << "\n";
    
    timeval thisSend;
    timeval lastDomainServerCheckIn = {};
    int numMicrosecondsSleep = 0;
    
    const long long DATA_SEND_INTERVAL_USECS = (1 / 60.0f) * 1000 * 1000;
    
    sockaddr_in senderAddress;
    unsigned char receivedData[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
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
        
        // allow the scripter's call back to setup visual data
        emit preSendCallback();
        // flush the voxel packet queue, don't allow it to sleep us
        voxelScripter.getVoxelPacketSender()->processWithoutSleep();
        
        while (NodeList::getInstance()->getNodeSocket()->receive((sockaddr*) &senderAddress, receivedData, &receivedBytes)) {
            NodeList::getInstance()->processNodeData((sockaddr*) &senderAddress, receivedData, receivedBytes);
        }
        
        // sleep for the correct amount of time to have data send be consistently timed
        if ((numMicrosecondsSleep = DATA_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&thisSend))) > 0) {
            usleep(numMicrosecondsSleep);
        }
    }
    
}
