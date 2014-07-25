//
//  Agent.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Agent_h
#define hifi_Agent_h

#include <vector>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <AvatarHashMap.h>
#include <ModelEditPacketSender.h>
#include <ModelTree.h>
#include <ModelTreeHeadlessViewer.h>
#include <ParticleEditPacketSender.h>
#include <ParticleTree.h>
#include <ParticleTreeHeadlessViewer.h>
#include <ScriptEngine.h>
#include <ThreadedAssignment.h>
#include <VoxelEditPacketSender.h>
#include <VoxelTreeHeadlessViewer.h>

#include "InboundMixedAudioStream.h"


class Agent : public ThreadedAssignment {
    Q_OBJECT
    
    Q_PROPERTY(bool isAvatar READ isAvatar WRITE setIsAvatar)
    Q_PROPERTY(bool isPlayingAvatarSound READ isPlayingAvatarSound)
    Q_PROPERTY(bool isListeningToAudioStream READ isListeningToAudioStream WRITE setIsListeningToAudioStream)
    Q_PROPERTY(float lastReceivedAudioLoudness READ getLastReceivedAudioLoudness)
public:
    Agent(const QByteArray& packet);
    
    void setIsAvatar(bool isAvatar) { QMetaObject::invokeMethod(&_scriptEngine, "setIsAvatar", Q_ARG(bool, isAvatar)); }
    bool isAvatar() const { return _scriptEngine.isAvatar(); }
    
    bool isPlayingAvatarSound() const  { return _scriptEngine.isPlayingAvatarSound(); }
    
    bool isListeningToAudioStream() const { return _scriptEngine.isListeningToAudioStream(); }
    void setIsListeningToAudioStream(bool isListeningToAudioStream)
        { _scriptEngine.setIsListeningToAudioStream(isListeningToAudioStream); }
    
    float getLastReceivedAudioLoudness() const { return _lastReceivedAudioLoudness; }

    virtual void aboutToFinish();
    
public slots:
    void run();
    void readPendingDatagrams();
    void playAvatarSound(Sound* avatarSound) { _scriptEngine.setAvatarSound(avatarSound); }

private:
    ScriptEngine _scriptEngine;
    VoxelEditPacketSender _voxelEditSender;
    ParticleEditPacketSender _particleEditSender;
    ModelEditPacketSender _modelEditSender;

    ParticleTreeHeadlessViewer _particleViewer;
    VoxelTreeHeadlessViewer _voxelViewer;
    ModelTreeHeadlessViewer _modelViewer;
    
    InboundMixedAudioStream _receivedAudioBuffer;
    float _lastReceivedAudioLoudness;

    AvatarHashMap _avatarHashMap;
};

#endif // hifi_Agent_h
