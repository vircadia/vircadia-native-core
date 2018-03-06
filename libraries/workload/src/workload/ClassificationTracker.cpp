//
//  ClassificationTracker.cpp
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.02.21
//  Copyright 2018 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple plane class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ClassificationTracker.h"

using namespace workload;

void ClassificationTracker::configure(const Config& config) {
}

void ClassificationTracker::run(const WorkloadContextPointer& context, Outputs& outputs) {
    auto space = context->_space;
    if (space) {
        Changes changes;
        space->categorizeAndGetChanges(changes);
        outputs.resize(workload::Region::NUM_TRANSITIONS);
        for (uint32_t i = 0; i < changes.size(); ++i) {
            int32_t j = Region::computeTransitionIndex(changes[i].prevRegion, changes[i].region);
            assert(j >= 0 && j < workload::Region::NUM_TRANSITIONS);
            outputs[j].push_back(changes[i].proxyId);
        }
    }
}

