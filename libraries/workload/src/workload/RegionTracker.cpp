//
//  RegionTracker.cpp
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
#include "RegionTracker.h"

#include "Region.h"

using namespace workload;

void RegionTracker::configure(const Config& config) {
}

void RegionTracker::run(const WorkloadContextPointer& context, Outputs& outputs) {
    outputs.clear();
    auto space = context->_space;
    if (space) {
        Changes changes;
        space->categorizeAndGetChanges(changes);
        // use exit/enter lists for each region less than Region::UNKNOWN
        outputs.resize(2 * (workload::Region::NUM_CLASSIFICATIONS - 1));
        for (uint32_t i = 0; i < changes.size(); ++i) {
            Space::Change& change = changes[i];
            if (change.prevRegion < Region::UNKNOWN) {
                // EXIT list index = 2 * regionIndex
                outputs[2 * change.prevRegion].push_back(change.proxyId);
            }
            if (change.region < Region::UNKNOWN) {
                // ENTER list index = 2 * regionIndex + 1
                outputs[2 * change.region + 1].push_back(change.proxyId);
            }
        }
    }
}
