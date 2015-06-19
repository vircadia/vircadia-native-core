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


EntityActionPointer InterfaceActionFactory::factory(EntitySimulation* simulation,
                                                    EntityActionType type,
                                                    QUuid id,
                                                    EntityItemPointer ownerEntity,
                                                    QVariantMap arguments) {
    EntityActionPointer action = nullptr;
    switch (type) {
        case ACTION_TYPE_NONE:
            return nullptr;
        case ACTION_TYPE_OFFSET:
            action = (EntityActionPointer) new ObjectActionOffset(id, ownerEntity);
            break;
        case ACTION_TYPE_SPRING:
            action = (EntityActionPointer) new ObjectActionSpring(id, ownerEntity);
            break;
        case ACTION_TYPE_HOLD:
            action = (EntityActionPointer) new AvatarActionHold(id, ownerEntity);
            break;
    }

    bool ok = action->updateArguments(arguments);
    if (ok) {
        ownerEntity->addAction(simulation, action);
        return action;
    }

    action = nullptr;
    return action;
}
