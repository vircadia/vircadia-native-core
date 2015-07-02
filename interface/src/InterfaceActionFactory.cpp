//
//  InterfaceActionFactory.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//



#include <avatar/AvatarActionHold.h>
#include <ObjectActionOffset.h>
#include <ObjectActionSpring.h>

#include "InterfaceActionFactory.h"


EntityActionPointer interfaceActionFactory(EntityActionType type, QUuid id, EntityItemPointer ownerEntity) {
    switch (type) {
        case ACTION_TYPE_NONE:
            return nullptr;
        case ACTION_TYPE_OFFSET:
            return (EntityActionPointer) new ObjectActionOffset(type, id, ownerEntity);
        case ACTION_TYPE_SPRING:
            return (EntityActionPointer) new ObjectActionSpring(type, id, ownerEntity);
        case ACTION_TYPE_HOLD:
            return (EntityActionPointer) new AvatarActionHold(type, id, ownerEntity);
    }

    assert(false);
    return nullptr;
}


EntityActionPointer InterfaceActionFactory::factory(EntitySimulation* simulation,
                                                    EntityActionType type,
                                                    QUuid id,
                                                    EntityItemPointer ownerEntity,
                                                    QVariantMap arguments) {
    EntityActionPointer action = interfaceActionFactory(type, id, ownerEntity);
    if (action) {
        bool ok = action->updateArguments(arguments);
        if (ok) {
            return action;
        }
    }
    return nullptr;
}


EntityActionPointer InterfaceActionFactory::factoryBA(EntitySimulation* simulation,
                                                      EntityItemPointer ownerEntity,
                                                      QByteArray data) {
    QDataStream serializedArgumentStream(data);
    EntityActionType type;
    QUuid id;

    serializedArgumentStream >> type;
    serializedArgumentStream >> id;

    EntityActionPointer action = interfaceActionFactory(type, id, ownerEntity);

    if (action) {
        action->deserialize(data);
    }
    return action;
}
