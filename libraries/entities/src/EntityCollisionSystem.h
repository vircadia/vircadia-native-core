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
#include <OctreePacketData.h>
#include <SharedUtil.h>

#include "EntityItem.h"
#include "SimpleEntitySimulation.h"

class AbstractAudioInterface;
class AvatarData;
class EntityEditPacketSender;
class EntityTree;

class EntityCollisionSystem : public SimpleEntitySimulation {
public:
    EntityCollisionSystem();

    void init(EntityEditPacketSender* packetSender, EntityTree* entities, AvatarHashMap* _avatars = NULL);
                                
    ~EntityCollisionSystem();

    void updateCollisions();

    void checkEntity(EntityItem* Entity);
    void updateCollisionWithEntities(EntityItem* Entity);
    void updateCollisionWithAvatars(EntityItem* Entity);

private:
    void applyHardCollision(EntityItem* entity, const CollisionInfo& collisionInfo);

    static bool updateOperation(OctreeElement* element, void* extraData);

    EntityEditPacketSender* _packetSender;
    AvatarHashMap* _avatars;
    CollisionList _collisions;
};

#endif // hifi_EntityCollisionSystem_h
