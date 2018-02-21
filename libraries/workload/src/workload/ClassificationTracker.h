//
//  ClassificationTracker.h
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

#ifndef hifi_workload_ClassificationTracker_h
#define hifi_workload_ClassificationTracker_h

#include "Space.h"
#include "Engine.h"

namespace workload {

    class ClassificationTrackerConfig : public Job::Config {
        Q_OBJECT
    public:
        ClassificationTrackerConfig() : Job::Config(true) {}
    };

    class ClassificationTracker {
    public:
        using Config = ClassificationTrackerConfig;
        using Outputs = Classifications;
        using JobModel = workload::Job::ModelO<ClassificationTracker, Outputs, Config>;

        ClassificationTracker() {}

        void configure(const Config& config);
        void run(const workload::WorkloadContextPointer& renderContext, Outputs& outputs);

    protected:
    };

} // namespace workload

#endif // hifi_workload_ClassificationTracker_h
