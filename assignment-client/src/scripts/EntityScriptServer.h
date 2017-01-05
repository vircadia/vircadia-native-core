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

#include <ThreadedAssignment.h>


class EntityScriptServer : public ThreadedAssignment {
    Q_OBJECT

public:
    EntityScriptServer(ReceivedMessage& message);

public slots:
    void run() override;
    void nodeActivated(SharedNodePointer activatedNode);
    void nodeKilled(SharedNodePointer killedNode);
    void sendStatsPacket() override;

private slots:
    void handleAudioPacket(QSharedPointer<ReceivedMessage> message);
    void handleOctreePacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleJurisdictionPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleSelectedAudioFormat(QSharedPointer<ReceivedMessage> message);
    
private:


};

#endif // hifi_EntityScriptServer_h
