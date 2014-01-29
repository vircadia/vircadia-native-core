//
//  Agent.h
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Agent__
#define __hifi__Agent__

#include <vector>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <ParticleTree.h>
#include <ScriptEngine.h>
#include <ThreadedAssignment.h>

class Agent : public ThreadedAssignment {
    Q_OBJECT
public:
    Agent(const QByteArray& packet);
    
public slots:
    void run();
    
    void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);
signals:
    void willSendAudioDataCallback();
    void willSendVisualDataCallback();
private:
    ScriptEngine _scriptEngine;
    ParticleTree _particleTree;
};

#endif /* defined(__hifi__Agent__) */
