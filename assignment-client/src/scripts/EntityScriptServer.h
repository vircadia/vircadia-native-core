//
//  EntityScriptServer.h
//  assignment-client/src/scripts
//
//  Created by Clément Brisset on 1/5/17.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityScriptServer_h
#define hifi_EntityScriptServer_h

#include <QtCore/QObject>

#include <EntityEditPacketSender.h>
#include <plugins/CodecPlugin.h>
#include <ThreadedAssignment.h>

#include "MixedAudioStream.h"

class EntityScriptServer : public ThreadedAssignment {
    Q_OBJECT

public:
    EntityScriptServer(ReceivedMessage& message);

    virtual void aboutToFinish() override;

public slots:
    void run() override;
    void nodeActivated(SharedNodePointer activatedNode);
    void nodeKilled(SharedNodePointer killedNode);
    void sendStatsPacket() override;

private slots:
    void handleOctreePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleJurisdictionPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message);
    void handleAudioPacket(QSharedPointer<ReceivedMessage> message);

private:
    void negotiateAudioFormat();
    void selectAudioFormat(const QString& selectedCodecName);

    EntityEditPacketSender _entityEditSender;

    QString _selectedCodecName;
    CodecPluginPointer _codec;
    Encoder* _encoder { nullptr };
    MixedAudioStream _receivedAudioStream;
    float _lastReceivedAudioLoudness;
};

#endif // hifi_EntityScriptServer_h
