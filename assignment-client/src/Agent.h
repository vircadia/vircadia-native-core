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
    Q_PROPERTY(bool isPlayingAvatarSound READ isPlayingAvatarSound)
    Q_PROPERTY(bool isListeningToAudioStream READ isListeningToAudioStream WRITE setIsListeningToAudioStream)
public:
    Agent(const QByteArray& packet);
    
    void setIsAvatar(bool isAvatar) { QMetaObject::invokeMethod(&_scriptEngine, "setIsAvatar", Q_ARG(bool, isAvatar)); }
    bool isAvatar() const { return _scriptEngine.isAvatar(); }
    
    bool isPlayingAvatarSound() const  { return _scriptEngine.isPlayingAvatarSound(); }
    
    bool isListeningToAudioStream() const { return _scriptEngine.isListeningToAudioStream(); }
    void setIsListeningToAudioStream(bool isListeningToAudioStream)
        { _scriptEngine.setIsListeningToAudioStream(isListeningToAudioStream); }

    virtual void aboutToFinish();
    
public slots:
    void run();
    void readPendingDatagrams();
    void playAvatarSound(Sound* avatarSound) { _scriptEngine.setAvatarSound(avatarSound); }

private:
    ScriptEngine _scriptEngine;
    VoxelEditPacketSender _voxelEditSender;
    ParticleEditPacketSender _particleEditSender;

    ParticleTreeHeadlessViewer _particleViewer;
    VoxelTreeHeadlessViewer _voxelViewer;
};

#endif /* defined(__hifi__Agent__) */
