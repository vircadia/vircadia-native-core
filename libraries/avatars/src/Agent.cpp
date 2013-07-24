//
//  Agent.cpp
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#import <QtScript/QScriptEngine>
#import <QtNetwork/QtNetwork>

#include <NodeList.h>

#include "AvatarData.h"

#include "Agent.h"

Agent::Agent() :
    _shouldStop(false)
{
    
}

void Agent::run(QUrl scriptURL) {
    NodeList::getInstance()->setOwnerType(NODE_TYPE_AGENT);
    NodeList::getInstance()->setNodeTypesOfInterest(&NODE_TYPE_AVATAR_MIXER, 1);
    
    QNetworkAccessManager* manager = new QNetworkAccessManager();
    
    qDebug() << "Attemping download of " << scriptURL;
    
    QNetworkReply* reply = manager->get(QNetworkRequest(scriptURL));
    
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
    
    QString scriptString = QString(reply->readAll());
    
    QScriptEngine engine;
    
    AvatarData *testAvatarData = new AvatarData;
    
    QScriptValue avatarDataValue = engine.newQObject(testAvatarData);
    engine.globalObject().setProperty("AvatarData", avatarDataValue);
    
    QScriptValue agentValue = engine.newQObject(this);
    engine.globalObject().setProperty("Agent", agentValue);
    
    qDebug() << "Downloaded script:" << scriptString;
    
    qDebug() << "Evaluated script:" << engine.evaluate(scriptString).toString();
    
    timeval thisSend;
    timeval lastDomainServerCheckIn = {};
    int numMicrosecondsSleep = 0;
    
    const float DATA_SEND_INTERVAL_MSECS = (1 / 60) * 1000;
    
    sockaddr_in senderAddress;
    unsigned char receivedData[MAX_PACKET_SIZE];
    ssize_t receivedBytes;
    
    while (!_shouldStop) {
        // update the thisSend timeval to the current time
        gettimeofday(&thisSend, NULL);
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
        }
        
        emit preSendCallback();
        
        testAvatarData->sendData();
        
        if (NodeList::getInstance()->getNodeSocket()->receive((sockaddr*) &senderAddress, receivedData, &receivedBytes)) {
            NodeList::getInstance()->processNodeData((sockaddr*) &senderAddress, receivedData, receivedBytes);
        }
        
        // sleep for the correct amount of time to have data send be consistently timed
        if ((numMicrosecondsSleep = (DATA_SEND_INTERVAL_MSECS * 1000) - (usecTimestampNow() - usecTimestamp(&thisSend))) > 0) {
            usleep(numMicrosecondsSleep);
        }
    }
    
}