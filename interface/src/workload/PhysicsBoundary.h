//
//  PhysicsBoundary.h
//
//  Created by Andrew Meadows 2018.04.05
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PhysicsBoundary_h
#define hifi_PhysicsBoundary_h

#include <workload/Engine.h>
#include <workload/RegionTracker.h>

class PhysicsBoundary {
public:
    using Config = workload::Job::Config;
    using Inputs = workload::RegionTracker::Outputs;
    using Outputs = bool;
    using JobModel = workload::Job::ModelI<PhysicsBoundary, Inputs, Config>;

    PhysicsBoundary() {}
    void configure(const Config& config) { }
    void run(const workload::WorkloadContextPointer& context, const Inputs& inputs);
};

#endif // hifi_PhysicsBoundary_h
