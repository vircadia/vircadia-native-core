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

    // How to make an Engine under the task::Task<C> paradigm...

    // (1) Derive class C from task::JobContext
    class WorkloadContext : public task::JobContext {
    public:
        WorkloadContext();
        virtual ~WorkloadContext() {}

        SpacePointer _space;
    };
    using WorkloadContextPointer = std::shared_ptr<WorkloadContext>;

    // (2) Apply a macro which will create local aliases (via "using") for example:
    //     using Task = task::Task<C>;
    Task_DeclareTypeAliases(WorkloadContext)

    // (3) You'll need a 'real Job' but it will need a Config for exposing settings to JS,
    // and you should do that here:
    class HelloWorldConfig : public Job::Config {
        Q_OBJECT
        Q_PROPERTY(QString message READ getMessage WRITE setMessage)
        QString _message {"Hello World."};
    public:
	    HelloWorldConfig() : Job::Config(true) {}
        QString getMessage() const { return _message; }
        void setMessage(const QString& msg) { _message = msg; }
    };

    // (4) In cpp file the 'real Job' will need a 'builder'.  The 'builder' is the 'Data' argument
    // for the Model template.
    // Data must implement Data::build().
    // Data::build() is called when the Model is added (in Engine ctor) as the first child job of the Engine

    // (5) Engine derives from task::Task<C> and will run all the Job<C>'s
    class Engine : public Task {
    public:
        Engine(const WorkloadContextPointer& context = std::make_shared<WorkloadContext>());
        ~Engine() = default;

        // (6) The Engine's Context is passed to its Jobs when they are run()
        void run() { assert(_context); Task::run(_context); }

        // Register the Space
        void registerSpace(const SpacePointer& space) { _context->_space = space; }

    protected:
        // (6) Again, the Engine's Context is passed to its Jobs when they are run()
        void run(const WorkloadContextPointer& context) override { assert(_context);  Task::run(_context); }

    private:
        WorkloadContextPointer _context;
    };
    using EnginePointer = std::shared_ptr<Engine>;

} // namespace workload

#endif // hifi_workload_Space_h
