//
//  EntityNodeData.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 4/29/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityNodeData_h
#define hifi_EntityNodeData_h

#include <udt/PacketHeaders.h>

#include <OctreeQueryNode.h>

namespace EntityJSONQueryProperties {
    static const QString SERVER_SCRIPTS_PROPERTY = "serverScripts";
    static const QString FLAGS_PROPERTY = "flags";
    static const QString INCLUDE_ANCESTORS_PROPERTY = "includeAncestors";
    static const QString INCLUDE_DESCENDANTS_PROPERTY = "includeDescendants";
}

class EntityNodeData : public OctreeQueryNode {
public:
    virtual PacketType getMyPacketType() const override { return PacketType::EntityData; }

    quint64 getLastDeletedEntitiesSentAt() const { return _lastDeletedEntitiesSentAt; }
    void setLastDeletedEntitiesSentAt(quint64 sentAt) { _lastDeletedEntitiesSentAt = sentAt; }
    
    // these can only be called from the OctreeSendThread for the given Node
    void insertSentFilteredEntity(const QUuid& entityID) { _sentFilteredEntities.insert(entityID); }
    void removeSentFilteredEntity(const QUuid& entityID) { _sentFilteredEntities.remove(entityID); }
    bool sentFilteredEntity(const QUuid& entityID) { return _sentFilteredEntities.contains(entityID); }
    QSet<QUuid> getSentFilteredEntities() { return _sentFilteredEntities; }

    // these can only be called from the OctreeSendThread for the given Node
    void insertFlaggedExtraEntity(const QUuid& filteredEntityID, const QUuid& extraEntityID)
        { _flaggedExtraEntities[filteredEntityID].insert(extraEntityID); }
    bool isEntityFlaggedAsExtra(const QUuid& entityID) const;

private:
    quint64 _lastDeletedEntitiesSentAt { usecTimestampNow() };
    QSet<QUuid> _sentFilteredEntities;
    QHash<QUuid, QSet<QUuid>> _flaggedExtraEntities;
};

#endif // hifi_EntityNodeData_h
