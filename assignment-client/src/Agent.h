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

#include <memory>
#include <vector>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QTimer>
#include <QUuid>

#include <EntityEditPacketSender.h>
#include <EntityTree.h>
#include <EntityTreeHeadlessViewer.h>
#include <ScriptEngine.h>
#include <ThreadedAssignment.h>

#include <plugins/CodecPlugin.h>

#include "MixedAudioStream.h"


class Agent : public ThreadedAssignment {
    Q_OBJECT

    Q_PROPERTY(bool isAvatar READ isAvatar WRITE setIsAvatar)
    Q_PROPERTY(bool isPlayingAvatarSound READ isPlayingAvatarSound)
    Q_PROPERTY(bool isListeningToAudioStream READ isListeningToAudioStream WRITE setIsListeningToAudioStream)
    Q_PROPERTY(float lastReceivedAudioLoudness READ getLastReceivedAudioLoudness)
    Q_PROPERTY(QUuid sessionUUID READ getSessionUUID)

public:
    Agent(ReceivedMessage& message);

    void setIsAvatar(bool isAvatar);
    bool isAvatar() const { return _isAvatar; }

    bool isPlayingAvatarSound() const { return _avatarSound != NULL; }

    bool isListeningToAudioStream() const { return _isListeningToAudioStream; }
    void setIsListeningToAudioStream(bool isListeningToAudioStream) { _isListeningToAudioStream = isListeningToAudioStream; }

    float getLastReceivedAudioLoudness() const { return _lastReceivedAudioLoudness; }
    QUuid getSessionUUID() const;

    virtual void aboutToFinish() override;

public slots:
    void run() override;
    void playAvatarSound(SharedSoundPointer avatarSound) { setAvatarSound(avatarSound); }

private slots:
    void requestScript();
    void scriptRequestFinished();
    void executeScript();

    void handleAudioPacket(QSharedPointer<ReceivedMessage> message);
    void handleOctreePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleJurisdictionPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message); 

    void processAgentAvatarAndAudio();
    void nodeActivated(SharedNodePointer activatedNode);

private:
    void negotiateAudioFormat();
    void selectAudioFormat(const QString& selectedCodecName);
    
    std::unique_ptr<ScriptEngine> _scriptEngine;
    EntityEditPacketSender _entityEditSender;
    EntityTreeHeadlessViewer _entityViewer;

    MixedAudioStream _receivedAudioStream;
    float _lastReceivedAudioLoudness;

    void setAvatarSound(SharedSoundPointer avatarSound) { _avatarSound = avatarSound; }

    void sendAvatarIdentityPacket();

    QString _scriptContents;
    QTimer* _scriptRequestTimeout { nullptr };
    ResourceRequest* _pendingScriptRequest { nullptr };
    bool _isListeningToAudioStream = false;
    SharedSoundPointer _avatarSound;
    int _numAvatarSoundSentBytes = 0;
    bool _isAvatar = false;
    QTimer* _avatarIdentityTimer = nullptr;
    QHash<QUuid, quint16> _outgoingScriptAudioSequenceNumbers;
    
    CodecPluginPointer _codec;
    QString _selectedCodecName;
    Encoder* _encoder { nullptr }; 
    QTimer* _avatarAudioTimer;
};

#endif // hifi_Agent_h
