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
#include "ClassificationTracker.h"

namespace workload {
    class DebugCout {
    public:
        using Inputs = SortedChanges;
        using JobModel = workload::Job::ModelI<DebugCout, Inputs>;

        DebugCout() {}

        void run(const workload::WorkloadContextPointer& renderContext, const Inputs& inputs) {
            qDebug() << "Some message from " << inputs.size();
            int i = 0;
            for (auto&  b: inputs) {
                qDebug() << "    Bucket Number" << i << " size is " << b.size();
                i++;
            }
        }

    protected:
    };


    WorkloadContext::WorkloadContext(const SpacePointer& space) : task::JobContext(trace_workload()), _space(space) {}

    class EngineBuilder {
    public:
        using Inputs = Views;
        using JobModel = Task::ModelI<EngineBuilder, Inputs>;
        void build(JobModel& model, const Varying& in, Varying& out) {
            model.addJob<SetupViews>("setupViews", in);
            const auto classifications = model.addJob<ClassificationTracker>("classificationTracker");
         //   model.addJob<DebugCout>("debug", classifications);

        }
    };

    Engine::Engine(const WorkloadContextPointer& context) : Task("Engine", EngineBuilder::JobModel::create()),
            _context(context) {
    }
} // namespace workload

