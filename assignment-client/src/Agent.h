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

#include <ParticleEditPacketSender.h>
#include <ParticleTree.h>
#include <ParticleTreeHeadlessViewer.h>
#include <ScriptEngine.h>
#include <ThreadedAssignment.h>
#include <VoxelEditPacketSender.h>
#include <VoxelTreeHeadlessViewer.h>


class Agent : public ThreadedAssignment {
    Q_OBJECT
    
    Q_PROPERTY(bool isAvatar READ isAvatar WRITE setIsAvatar)
    Q_PROPERTY(bool sendAvatarAudioStream READ isSendingAvatarAudioStream WRITE setSendAvatarAudioStream)
public:
    Agent(const QByteArray& packet);
    ~Agent();
    
    void setIsAvatar(bool isAvatar) { QMetaObject::invokeMethod(&_scriptEngine, "setIsAvatar", Q_ARG(bool, isAvatar)); }
    bool isAvatar() const { return _scriptEngine.isAvatar(); }
    
    void setSendAvatarAudioStream(bool sendAvatarAudioStream);
    bool isSendingAvatarAudioStream() const { return (bool) _scriptEngine.sendsAvatarAudioStream(); }
    
public slots:
    void run();
    void readPendingDatagrams();

private:
    ScriptEngine _scriptEngine;
    VoxelEditPacketSender _voxelEditSender;
    ParticleEditPacketSender _particleEditSender;

    ParticleTreeHeadlessViewer _particleViewer;
    VoxelTreeHeadlessViewer _voxelViewer;
    
    int16_t* _avatarAudioStream;
};

#endif /* defined(__hifi__Agent__) */
