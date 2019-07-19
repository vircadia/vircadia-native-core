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
    auto& outChanges = outputs.edit0();
    auto& outRegionChanges = outputs.edit1();


    outChanges.clear();
    outRegionChanges.clear();

    auto space = context->_space;
    if (space) {
        //Changes changes;
        space->categorizeAndGetChanges(outChanges);

        // use exit/enter lists for each region less than Region::R4
        outRegionChanges.resize(2 * workload::Region::NUM_TRACKED_REGIONS);
        for (uint32_t i = 0; i < outChanges.size(); ++i) {
            Space::Change& change = outChanges[i];
            if (change.prevRegion < Region::R4) {
                // EXIT list index = 2 * regionIndex
                outRegionChanges[2 * change.prevRegion].push_back(change.proxyId);
            }
            if (change.region < Region::R4) {
                // ENTER list index = 2 * regionIndex + 1
                outRegionChanges[2 * change.region + 1].push_back(change.proxyId);
            }
        }
    }
}
