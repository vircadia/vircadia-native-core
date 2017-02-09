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
#include <EntityTreeHeadlessViewer.h>
#include <plugins/CodecPlugin.h>
#include <ScriptEngine.h>
#include <ThreadedAssignment.h>

static const int DEFAULT_MAX_ENTITY_PPS = 9000;
static const int DEFAULT_ENTITY_PPS_PER_SCRIPT = 900;

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

    void handleReloadEntityServerScriptPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleEntityScriptGetStatusPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);

    void handleSettings();
    void updateEntityPPS();

private:
    void negotiateAudioFormat();
    void selectAudioFormat(const QString& selectedCodecName);

    void resetEntitiesScriptEngine();
    void clear();
    void shutdownScriptEngine();

    void addingEntity(const EntityItemID& entityID);
    void deletingEntity(const EntityItemID& entityID);
    void entityServerScriptChanging(const EntityItemID& entityID, const bool reload);
    void checkAndCallPreload(const EntityItemID& entityID, const bool reload = false);

    bool _shuttingDown { false };

    static int _entitiesScriptEngineCount;
    QSharedPointer<ScriptEngine> _entitiesScriptEngine;
    EntityEditPacketSender _entityEditSender;
    EntityTreeHeadlessViewer _entityViewer;

    int _maxEntityPPS { DEFAULT_MAX_ENTITY_PPS };
    int _entityPPSPerScript { DEFAULT_ENTITY_PPS_PER_SCRIPT };

    QString _selectedCodecName;
    CodecPluginPointer _codec;
    Encoder* _encoder { nullptr };
};

#endif // hifi_EntityScriptServer_h
