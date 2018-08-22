//
//  PhysicsBoundary.h
//
//  Created by Andrew Meadows 2018.04.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsBoundary.h"

#include <PhysicsLogging.h>
#include <workload/Space.h>

#include "workload/GameWorkload.h"

void PhysicsBoundary::run(const workload::WorkloadContextPointer& context, const Inputs& inputs) {
    auto space = context->_space;
    if (!space) {
        return;
    }
    GameWorkloadContext* gameContext = static_cast<GameWorkloadContext*>(context.get());
    PhysicalEntitySimulationPointer simulation = gameContext->_simulation;
    const auto& regionChanges = inputs.get0();
    for (uint32_t i = 0; i < (uint32_t)regionChanges.size(); ++i) {
        const workload::Space::Change& change = regionChanges[i];
        auto entity = space->getOwner(change.proxyId).get<EntityItemPointer>();
        if (entity) {
            simulation->changeEntity(entity);
        }
    }
}
