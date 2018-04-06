//
//  PhysicsBoundary.h
//
//  Created by Sam Gateau on 2/16/2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PhysicsGatekeeper_h
#define hifi_PhysicsGatekeeper_h

#include "workload/Space.h"
#include "workload/Engine.h"

#include "render/Scene.h"

#include <workload/RegionTracker.h>
#include <EntityItem.h>
#include "PhysicalEntitySimulation.h"

class PhysicsBoundary {
public:
    using Config = workload::Job::Config;
    using Inputs = workload::RegionTracker::Outputs;
    using Outputs = bool;
    using JobModel = workload::Job::ModelI<PhysicsBoundary, Inputs, Config>; // this doesn't work

    PhysicsBoundary() {}

    void configure(const Config& config) {
    }

    void run(const workload::WorkloadContextPointer& context, const Inputs& inputs) {
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
                qCDebug("physics") << change.proxyId << " : " << "'" << entity->getName() << "' "
                    << (uint32_t)(change.prevRegion) << " --> " << (uint32_t)(change.region);
            }
        }
    }
};

#endif // hifi_PhysicsGatekeeper_h
