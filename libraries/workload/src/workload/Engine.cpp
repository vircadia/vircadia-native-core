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

namespace workload {

    // the "real Job"
    class HelloWorld {
        QString _message;
        bool _isEnabled { true };
    public:
        using JobModel = Job::Model<HelloWorld, HelloWorldConfig>;
        HelloWorld() {}
        void configure(const HelloWorldConfig& configuration) {
            _isEnabled = configuration.isEnabled();
            _message = configuration.getMessage();
        }
        void run(const WorkloadContextPointer& context) {
            if (_isEnabled) {
                std::cout << _message.toStdString() << std::endl;
            }
        }
    };

    WorkloadContext::WorkloadContext(const SpacePointer& space) : task::JobContext(trace_workload()), _space(space) {}

    using EngineModel = Task::Model<class HelloWorldBuilder>;

    // the 'Builder' is the 'Data' on which the EngineModel templatizes.
    // It must implement build() which is called by EngineModel::create().
    class HelloWorldBuilder {
    public:
        using JobModel = Task::Model<EngineModel>;
        void build(EngineModel& model, const Varying& in, Varying& out) {
            model.addJob<HelloWorld>("helloWorld");
        }
    };

    Engine::Engine(const WorkloadContextPointer& context) : Task("Engine", EngineModel::create()),
            _context(context) {
    }
} // namespace workload

