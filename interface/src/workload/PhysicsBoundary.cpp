//
//  PhysicsBoundary.cpp
//
//  Created by Andrew Meadows 2018.04.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsBoundary.h"

#include <EntityItem.h>
#include <PhysicalEntitySimulation.h>
#include <PhysicsLogging.h>
#include <workload/Space.h>

#include "avatar/AvatarManager.h"
#include "avatar/OtherAvatar.h"
#include "workload/GameWorkload.h"

void PhysicsBoundary::run(const workload::WorkloadContextPointer& context, const Inputs& inputs) {
    auto space = context->_space;
    if (!space) {
        return;
    }
    GameWorkloadContext* gameContext = static_cast<GameWorkloadContext*>(context.get());
    const auto& regionChanges = inputs.get0();
    for (uint32_t i = 0; i < (uint32_t)regionChanges.size(); ++i) {
        const workload::Space::Change& change = regionChanges[i];
        auto nestable = space->getOwner(change.proxyId).get<SpatiallyNestablePointer>();
        if (nestable) {
            switch (nestable->getNestableType()) {
                case NestableType::Entity: {
                    gameContext->_simulation->changeEntity(std::static_pointer_cast<EntityItem>(nestable));
                }
                break;
                case NestableType::Avatar: {
                    auto avatar = std::static_pointer_cast<OtherAvatar>(nestable);
                    avatar->setWorkloadRegion(change.region);
                    if (avatar->isInPhysicsSimulation() != avatar->shouldBeInPhysicsSimulation()) {
                        DependencyManager::get<AvatarManager>()->queuePhysicsChange(avatar);
                    }
                }
                break;
                default:
                break;
            }
        }
    }
}
