//
//  Engine.h
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

#ifndef hifi_workload_Engine_h
#define hifi_workload_Engine_h

#include <QtCore/QObject>
#include <QLoggingCategory>

#include <task/Task.h>

#include "Space.h"

namespace workload {
    class WorkloadContext : public task::JobContext {
    public:
        WorkloadContext(const SpacePointer& space);
        virtual ~WorkloadContext() {}

        SpacePointer _space;
    };

    using WorkloadContextPointer = std::shared_ptr<WorkloadContext>;
    Task_DeclareTypeAliases(WorkloadContext)

    class Engine {
    public:
        Engine(const std::shared_ptr<Task>& task) : _task(task), _context(nullptr) {
        }
        ~Engine() = default;

        void reset(const WorkloadContextPointer& context);

        void run() { if (_context) { _task->run(_context); } }

        std::shared_ptr<TaskConfig> getConfiguration() { return _task->getConfiguration(); }

        std::shared_ptr<Task> _task;

    protected:

    private:
        WorkloadContextPointer _context;
    };
    using EnginePointer = std::shared_ptr<Engine>;
} // namespace workload

#endif // hifi_workload_Space_h
