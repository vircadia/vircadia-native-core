//
//  Engine.cpp
//  libraries/workload/src/workload
//
//  Created by Andrew Meadows 2018.02.08
//  Copyright 2018 High Fidelity, Inc.
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
        using Outputs = RegionTracker::Outputs;
        using JobModel = Task::ModelIO<EngineBuilder, Inputs, Outputs>;
        void build(JobModel& model, const Varying& in, Varying& out) {
            model.addJob<SetupViews>("setupViews", in);
            model.addJob<PerformSpaceTransaction>("updateSpace");
            const auto regionTrackerOut = model.addJob<RegionTracker>("regionTracker");
            const auto regionChanges = regionTrackerOut.getN<RegionTracker::Outputs>(1);
            model.addJob<RegionState>("regionState", regionChanges);
            out = regionTrackerOut;
        }
    };

    Engine::Engine() : Task("Engine", EngineBuilder::JobModel::create()),
            _context(nullptr) {
    }

    void Engine::reset(const WorkloadContextPointer& context) {
        _context = context;
    }

    void PerformSpaceTransaction::configure(const Config& config) {

    }
    void PerformSpaceTransaction::run(const WorkloadContextPointer& context) {
        context->_space->enqueueFrame();
        context->_space->processTransactionQueue();
    }

} // namespace workload

