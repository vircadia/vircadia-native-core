//
//  EntityCollisionSystem.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 9/23/14.
//  Copyright 2013-2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityCollisionSystem_h
#define hifi_EntityCollisionSystem_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <AvatarHashMap.h>
#include <CollisionInfo.h>
#include <SharedUtil.h>
#include <OctreePacketData.h>
#include <VoxelDetail.h>

#include "EntityItem.h"

class AbstractAudioInterface;
class AvatarData;
class EntityEditPacketSender;
class EntityTree;
class VoxelTree;

class EntityCollisionSystem : public QObject {
Q_OBJECT
public:
    EntityCollisionSystem(EntityEditPacketSender* packetSender = NULL, EntityTree* Entitys = NULL, 
                                VoxelTree* voxels = NULL, AbstractAudioInterface* audio = NULL, 
                                AvatarHashMap* avatars = NULL);

    void init(EntityEditPacketSender* packetSender, EntityTree* Entitys, VoxelTree* voxels, 
                                AbstractAudioInterface* audio = NULL, AvatarHashMap* _avatars = NULL);
                                
    ~EntityCollisionSystem();

    void update();

    void checkEntity(EntityItem* Entity);
    void updateCollisionWithVoxels(EntityItem* Entity);
    void updateCollisionWithEntities(EntityItem* Entity);
    void updateCollisionWithAvatars(EntityItem* Entity);
    void queueEntityPropertiesUpdate(EntityItem* Entity);
    void updateCollisionSound(EntityItem* Entity, const glm::vec3 &penetration, float frequency);

signals:
    void entityCollisionWithVoxel(const EntityItemID& entityItemID, const VoxelDetail& voxel, const CollisionInfo& penetration);
    void entityCollisionWithEntity(const EntityItemID& idA, const EntityItemID& idB, const CollisionInfo& penetration);

private:
    void applyHardCollision(EntityItem* entity, const CollisionInfo& collisionInfo);

    static bool updateOperation(OctreeElement* element, void* extraData);
    void emitGlobalEntityCollisionWithVoxel(EntityItem* Entity, VoxelDetail* voxelDetails, const CollisionInfo& penetration);
    void emitGlobalEntityCollisionWithEntity(EntityItem* entityA, EntityItem* entityB, const CollisionInfo& penetration);

    EntityEditPacketSender* _packetSender;
    EntityTree* _entities;
    VoxelTree* _voxels;
    AbstractAudioInterface* _audio;
    AvatarHashMap* _avatars;
    CollisionList _collisions;
};

#endif // hifi_EntityCollisionSystem_h
