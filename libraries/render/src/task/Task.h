//
//  Task.h
//  render/src/task
//
//  Created by Zach Pomerantz on 1/6/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_task_Task_h
#define hifi_task_Task_h

#include "Config.h"
#include "Varying.h"

#include "SettingHandle.h"

#include "Logging.h"

#include <Profile.h>
#include <PerfStat.h>

namespace task {

class JobConcept;
template <class RC> class JobT;
template <class RC> class TaskT;
class JobNoIO {};

class JobContext {
public:
    virtual ~JobContext() {}

    std::shared_ptr<JobConfig> jobConfig { nullptr };
};
using JobContextPointer = std::shared_ptr<JobContext>;

// The guts of a job
class JobConcept {
public:
    using Config = JobConfig;

    JobConcept(QConfigPointer config) : _config(config) {}
    virtual ~JobConcept() = default;

    virtual const Varying getInput() const { return Varying(); }
    virtual const Varying getOutput() const { return Varying(); }

    virtual QConfigPointer& getConfiguration() { return _config; }
    virtual void applyConfiguration() = 0;

    void setCPURunTime(double mstime) { std::static_pointer_cast<Config>(_config)->setCPURunTime(mstime); }

    QConfigPointer _config;
protected:
};


template <class T, class C> void jobConfigure(T& data, const C& configuration) {
    data.configure(configuration);
}
template<class T> void jobConfigure(T&, const JobConfig&) {
    // nop, as the default JobConfig was used, so the data does not need a configure method
}
template<class T> void jobConfigure(T&, const TaskConfig&) {
    // nop, as the default TaskConfig was used, so the data does not need a configure method
}

template <class T, class RC> void jobRun(T& data, const RC& renderContext, const JobNoIO& input, JobNoIO& output) {
    data.run(renderContext);
}
template <class T, class RC, class I> void jobRun(T& data, const RC& renderContext, const I& input, JobNoIO& output) {
    data.run(renderContext, input);
}
template <class T, class RC, class O> void jobRun(T& data, const RC& renderContext, const JobNoIO& input, O& output) {
    data.run(renderContext, output);
}
template <class T, class RC, class I, class O> void jobRun(T& data, const RC& renderContext, const I& input, O& output) {
    data.run(renderContext, input, output);
}

template <class RC>
class Job {
public:
    using Context = RC;
    using ContextPointer = std::shared_ptr<Context>;
    using Config = JobConfig;
    using None = JobNoIO;

    class Concept : public JobConcept {
    public:
        Concept(QConfigPointer config) : JobConcept(config) {}
        virtual ~Concept() = default;

        virtual void run(const ContextPointer& renderContext) = 0;
    };
    using ConceptPointer = std::shared_ptr<Concept>;

    template <class T, class C = Config, class I = None, class O = None> class Model : public Concept {
    public:
        using Data = T;
        using Input = I;
        using Output = O;

        Data _data;
        Varying _input;
        Varying _output;

        const Varying getInput() const override { return _input; }
        const Varying getOutput() const override { return _output; }

        template <class... A>
        Model(const Varying& input, QConfigPointer config, A&&... args) :
            Concept(config),
            _data(Data(std::forward<A>(args)...)),
            _input(input),
            _output(Output()) {
            applyConfiguration();
        }

        template <class... A>
        static std::shared_ptr<Model> create(const Varying& input, A&&... args) {
            return std::make_shared<Model>(input, std::make_shared<C>(), std::forward<A>(args)...);
        }


        void applyConfiguration() override {
            jobConfigure(_data, *std::static_pointer_cast<C>(Concept::_config));
        }

        void run(const ContextPointer& renderContext) override {
            renderContext->jobConfig = std::static_pointer_cast<Config>(Concept::_config);
            if (renderContext->jobConfig->alwaysEnabled || renderContext->jobConfig->isEnabled()) {
                jobRun(_data, renderContext, _input.get<I>(), _output.edit<O>());
            }
            renderContext->jobConfig.reset();
        }
    };
    template <class T, class I, class C = Config> using ModelI = Model<T, C, I, None>;
    template <class T, class O, class C = Config> using ModelO = Model<T, C, None, O>;
    template <class T, class I, class O, class C = Config> using ModelIO = Model<T, C, I, O>;

    Job(std::string name, ConceptPointer concept) : _concept(concept), _name(name) {}

    const Varying getInput() const { return _concept->getInput(); }
    const Varying getOutput() const { return _concept->getOutput(); }
    QConfigPointer& getConfiguration() const { return _concept->getConfiguration(); }
    void applyConfiguration() { return _concept->applyConfiguration(); }

    template <class T> T& edit() {
        auto concept = std::static_pointer_cast<typename T::JobModel>(_concept);
        assert(concept);
        return concept->_data;
    }

    template <class T> const T& get() const {
        auto concept = std::static_pointer_cast<typename T::JobModel>(_concept);
        assert(concept);
        return concept->_data;
    }

    virtual void run(const ContextPointer& renderContext) {
        PerformanceTimer perfTimer(_name.c_str());
        PROFILE_RANGE(render, _name.c_str());
        auto start = usecTimestampNow();

        _concept->run(renderContext);

        _concept->setCPURunTime((double)(usecTimestampNow() - start) / 1000.0);
    }

protected:
    ConceptPointer _concept;
    std::string _name = "";
};


// A task is a specialized job to run a collection of other jobs
// It can be created on any type T by aliasing the type JobModel in the class T
// using JobModel = Task::Model<T>
// The class T is expected to have a "build" method acting as a constructor.
// The build method is where child Jobs can be added internally to the task 
// where the input of the task can be setup to feed the child jobs
// and where the output of the task is defined
template <class RC>
class Task : public Job<RC> {
public:
    using Context = RC;
    using ContextPointer = std::shared_ptr<Context>;
    using Config = TaskConfig;
    using JobType = Job<RC>;
    using None = typename JobType::None;
    using Concept = typename JobType::Concept;
    using ConceptPointer = typename JobType::ConceptPointer;
    using Jobs = std::vector<JobType>;

    Task(std::string name, ConceptPointer concept) : JobType(name, concept) {}

    class TaskConcept : public Concept {
    public:
        Varying _input;
        Varying _output;
        Jobs _jobs;

        const Varying getInput() const override { return _input; }
        const Varying getOutput() const override { return _output; }

        TaskConcept(const Varying& input, QConfigPointer config) : Concept(config), _input(input) {}

        // Create a new job in the container's queue; returns the job's output
        template <class NT, class... NA> const Varying addJob(std::string name, const Varying& input, NA&&... args) {
            _jobs.emplace_back(name, (NT::JobModel::create(input, std::forward<NA>(args)...)));

            // Conect the child config to this task's config
            std::static_pointer_cast<TaskConfig>(Concept::getConfiguration())->connectChildConfig(_jobs.back().getConfiguration(), name);

            return _jobs.back().getOutput();
        }
        template <class NT, class... NA> const Varying addJob(std::string name, NA&&... args) {
            const auto input = Varying(typename NT::JobModel::Input());
            return addJob<NT>(name, input, std::forward<NA>(args)...);
        }
    };

    template <class T, class C = Config, class I = None, class O = None> class TaskModel : public TaskConcept {
    public:
        using Data = T;
        using Input = I;
        using Output = O;

        Data _data;

        TaskModel(const Varying& input, QConfigPointer config) :
            TaskConcept(input, config),
            _data(Data()) {}

        template <class... A>
        static std::shared_ptr<TaskModel> create(const Varying& input, A&&... args) {
            auto model = std::make_shared<TaskModel>(input, std::make_shared<C>());

            model->_data.build(*(model), model->_input, model->_output, std::forward<A>(args)...);

            // Recreate the Config to use the templated type
            model->createConfiguration();
            model->applyConfiguration();

            return model;
        }

        template <class... A>
        static std::shared_ptr<TaskModel> create(A&&... args) {
            const auto input = Varying(Input());
            return create(input, std::forward<A>(args)...);
        }

        void createConfiguration() {
            // A brand new config
            auto config = std::make_shared<C>();
            // Make sure we transfer the former children configs to the new config
            config->transferChildrenConfigs(Concept::_config);
            // swap
            Concept::_config = config;
            // Capture this
            std::static_pointer_cast<C>(Concept::_config)->_task = this;
        }

        QConfigPointer& getConfiguration() override {
            if (!Concept::_config) {
                createConfiguration();
            }
            return Concept::_config;
        }

        void applyConfiguration() override {
            jobConfigure(_data, *std::static_pointer_cast<C>(Concept::_config));
            for (auto& job : TaskConcept::_jobs) {
                job.applyConfiguration();
            }
        }

        void run(const ContextPointer& renderContext) override {
            auto config = std::static_pointer_cast<C>(Concept::_config);
            if (config->alwaysEnabled || config->enabled) {
                for (auto job : TaskConcept::_jobs) {
                    job.run(renderContext);
                }
            }
        }
    };
    template <class T, class C = Config> using Model = TaskModel<T, C, None, None>;
    template <class T, class I, class C = Config> using ModelI = TaskModel<T, C, I, None>;
    template <class T, class O, class C = Config> using ModelO = TaskModel<T, C, None, O>;
    template <class T, class I, class O, class C = Config> using ModelIO = TaskModel<T, C, I, O>;

    // Create a new job in the Task's queue; returns the job's output
    template <class T, class... A> const Varying addJob(std::string name, const Varying& input, A&&... args) {
        return std::static_pointer_cast<TaskConcept>(JobType::_concept)->template addJob<T>(name, input, std::forward<A>(args)...);
    }
    template <class T, class... A> const Varying addJob(std::string name, A&&... args) {
        const auto input = Varying(typename T::JobModel::Input());
        return std::static_pointer_cast<TaskConcept>(JobType::_concept)->template addJob<T>(name, input, std::forward<A>(args)...);
    }

    std::shared_ptr<Config> getConfiguration() {
        return std::static_pointer_cast<Config>(JobType::_concept->getConfiguration());
    }

protected:
};
}


#define Task_DeclareTypeAliases(ContextType) \
    using JobConfig = task::JobConfig; \
    using TaskConfig = task::TaskConfig; \
    template <class T> using PersistentConfig = task::PersistentConfig<T>; \
    using Job = task::Job<ContextType>; \
    using Task = task::Task<ContextType>; \
    using Varying = task::Varying; \
    template < typename T0, typename T1 > using VaryingSet2 = task::VaryingSet2<T0, T1>; \
    template < typename T0, typename T1, typename T2 > using VaryingSet3 = task::VaryingSet3<T0, T1, T2>; \
    template < typename T0, typename T1, typename T2, typename T3 > using VaryingSet4 = task::VaryingSet4<T0, T1, T2, T3>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4 > using VaryingSet5 = task::VaryingSet5<T0, T1, T2, T3, T4>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5 > using VaryingSet6 = task::VaryingSet6<T0, T1, T2, T3, T4, T5>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 > using VaryingSet7 = task::VaryingSet7<T0, T1, T2, T3, T4, T5, T6>; \
    template < class T, int NUM > using VaryingArray = task::VaryingArray<T, NUM>;

#endif // hifi_task_Task_h
