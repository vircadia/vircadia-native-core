//
//  Engine.cpp
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.02.08
//  Copyright 2018 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple plane class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Engine.h"

#include <iostream>

#include "ViewTask.h"
#include "RegionTracker.h"
#include "RegionState.h"

namespace workload {
    WorkloadContext::WorkloadContext(const SpacePointer& space) : task::JobContext(trace_workload()), _space(space) {}

    class EngineBuilder {
    public:
        using Inputs = Views;
        using JobModel = Task::ModelI<EngineBuilder, Inputs>;
        void build(JobModel& model, const Varying& in, Varying& out) {
            model.addJob<SetupViews>("setupViews", in);
            const auto regionChanges = model.addJob<RegionTracker>("regionTracker");
            model.addJob<RegionState>("regionState", regionChanges);

        }
    };

    Engine::Engine(const WorkloadContextPointer& context) : Task("Engine", EngineBuilder::JobModel::create()),
            _context(context) {
    }
} // namespace workload

