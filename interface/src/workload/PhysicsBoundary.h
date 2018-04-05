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

/*
class FooJobConfig : public workload::Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool fubar MEMBER fubar NOTIFY dirty)
public:
    bool fubar{ false };
signals:
    void dirty();
protected:
};
*/

#include <iostream> // adebug
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
        uint32_t listSize = inputs.size();
        uint32_t totalTransitions = 0;
        for (uint32_t i = 0; i < listSize; ++i) {
            totalTransitions += inputs[i].size();
        }
        // we're interested in things entering/leaving R3
        uint32_t regionIndex = workload::Region::R3;
        uint32_t exitIndex = 2 * regionIndex;
        uint32_t numExits = inputs[exitIndex].size();
        for (uint32_t i = 0; i < numExits; ++i) {
            int32_t proxyID = inputs[exitIndex][i];
            void* owner = space->getOwner(proxyID).get();
            if (owner) {
                EntityItem* entity = static_cast<EntityItem*>(owner);
                std::cout << "adebug  - "
                    //<< owner
                    << "  '" << entity->getName().toStdString() << "'"
                    << std::endl;     // adebug
            }
        }

        uint32_t enterIndex = exitIndex + 1;
        uint32_t numEntries = inputs[enterIndex].size();
        for (uint32_t i = 0; i < numEntries; ++i) {
            int32_t proxyID = inputs[enterIndex][i];
            void* owner = space->getOwner(proxyID).get();
            if (owner) {
                EntityItem* entity = static_cast<EntityItem*>(owner);
                std::cout << "adebug  + "
                    //<< owner
                    << "  '" << entity->getName().toStdString() << "'"
                    << std::endl;     // adebug
            }
        }
    }
};

#endif // hifi_PhysicsGatekeeper_h
