//
//  RegionTracker.h
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

#ifndef hifi_workload_RegionTracker_h
#define hifi_workload_RegionTracker_h

#include "Space.h"
#include "Engine.h"

namespace workload {

    class RegionTrackerConfig : public Job::Config {
        Q_OBJECT
    public:
        RegionTrackerConfig() : Job::Config(true) {}
    };

    class RegionTracker {
    public:
        using Config = RegionTrackerConfig;
        using Outputs = VaryingSet2<Changes, IndexVectors>;
        using JobModel = workload::Job::ModelO<RegionTracker, Outputs, Config>;

        RegionTracker() {}

        void configure(const Config& config);
        void run(const workload::WorkloadContextPointer& renderContext, Outputs& outputs);

    protected:
    };
} // namespace workload

#endif // hifi_workload_RegionTracker_h
