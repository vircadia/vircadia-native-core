//
//  SpaceClassifier.h
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.02.21
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_workload_SpaceClassifier_h
#define hifi_workload_SpaceClassifier_h

#include "ViewTask.h"
#include "RegionTracker.h"

namespace workload {
    class SpaceClassifierTask {
    public:
        using Inputs = Views;
        using Outputs = RegionTracker::Outputs;
        using JobModel = Task::ModelIO<SpaceClassifierTask, Inputs, Outputs>;
        void build(JobModel& model, const Varying& in, Varying& out);
    };


    class PerformSpaceTransactionConfig : public Job::Config {
        Q_OBJECT
    public:
    signals :
        void dirty();

    protected:
    };

    class PerformSpaceTransaction {
    public:
        using Config = PerformSpaceTransactionConfig;
        using JobModel = Job::Model<PerformSpaceTransaction, Config>;

        void configure(const Config& config);
        void run(const WorkloadContextPointer& context);
    protected:
    };
} // namespace workload

#endif // hifi_workload_SpaceClassifier_h
