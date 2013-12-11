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

#include <ThreadedAssignment.h>

#include <VoxelScriptingInterface.h>
#include <ParticleScriptingInterface.h>

class Agent : public ThreadedAssignment {
    Q_OBJECT
public:
    Agent(const unsigned char* dataBuffer, int numBytes);
    
public slots:
    void run();
    
    void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);
signals:
    void willSendAudioDataCallback();
    void willSendVisualDataCallback();
private:
    VoxelScriptingInterface _voxelScriptingInterface;
    ParticleScriptingInterface _particleScriptingInterface;
};

#endif /* defined(__hifi__Agent__) */
