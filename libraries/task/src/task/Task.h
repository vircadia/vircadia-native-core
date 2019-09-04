//
//  Task.h
//  task/src/task
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

#include <unordered_map>

namespace task {

class JobConcept;
template <class JC, class TP> class JobT;
template <class JC, class TP> class TaskT;
class JobNoIO {};

// Task Flow control class is a simple per value object used to communicate flow control commands trhough the graph of tasks.
// From within the Job::Run function, you can access it from the JobContext and  issue commands which   will be picked up by the Task calling for the Job run.
// This is first introduced to provide a way to abort all the work from within a task job. see the "abortTask" call
class TaskFlow {
public:
    // A job that wants to abort the rest of the other jobs execution in a task would issue that call "abortTask" and let the task early exit for this run.
    // All the varyings produced by the aborted branch of jobs are left unmodified.
    void abortTask();

    // called by the task::run to perform flow control
    // This should be considered private but still need to be accessible from the Task<T> class
    TaskFlow() = default;
    ~TaskFlow() = default;
    void reset();
    bool doAbortTask() const;

protected:
    bool _doAbortTask{ false };
};

// JobContext class is the base class for the context object which is passed through all the Job::run calls thoughout the graph of jobs
// It is used to communicate to the job::run its context and various state information the job relies on.
// It specifically provide access to:
// - The taskFlow object allowing for messaging control flow commands from within a Job::run
// - The current Config object attached to the Job::run currently called.
// The JobContext can be derived to add more global state to it that Jobs can access
class JobContext {
public:
    JobContext();
    virtual ~JobContext();

    std::shared_ptr<JobConfig> jobConfig { nullptr };

    // Task flow control
    TaskFlow taskFlow{};

protected:
};
using JobContextPointer = std::shared_ptr<JobContext>;

// The guts of a job
class JobConcept {
public:
    using Config = JobConfig;

    JobConcept(const std::string& name, QConfigPointer config) : _config(config), _name(name) { config->_jobConcept = this; }
    virtual ~JobConcept() = default;
    
    const std::string& getName() const { return _name; }

    virtual const Varying getInput() const { return Varying(); }
    virtual const Varying getOutput() const { return Varying(); }
    virtual Varying& editInput() = 0;

    virtual QConfigPointer& getConfiguration() { return _config; }
    virtual void applyConfiguration() = 0;
    void setCPURunTime(const std::chrono::nanoseconds& runtime) { (_config)->setCPURunTime(runtime); }

    QConfigPointer _config;
protected:
    const std::string _name;
};


template <class T, class C> void jobConfigure(T& data, const C& configuration) {
    data.configure(configuration);
}
template<class T> void jobConfigure(T&, const JobConfig&) {
    // nop, as the default JobConfig was used, so the data does not need a configure method
}

template <class T, class JC> void jobRun(T& data, const JC& jobContext, const JobNoIO& input, JobNoIO& output) {
    data.run(jobContext);
}
template <class T, class JC, class I> void jobRun(T& data, const JC& jobContext, const I& input, JobNoIO& output) {
    data.run(jobContext, input);
}
template <class T, class JC, class O> void jobRun(T& data, const JC& jobContext, const JobNoIO& input, O& output) {
    data.run(jobContext, output);
}
template <class T, class JC, class I, class O> void jobRun(T& data, const JC& jobContext, const I& input, O& output) {
    data.run(jobContext, input, output);
}

template <class JC, class TP>
class Job {
public:
    using Context = JC;
    using TimeProfiler = TP;
    using ContextPointer = std::shared_ptr<Context>;
    using Config = JobConfig;
    using None = JobNoIO;

    class Concept : public JobConcept {
    public:
        Concept(const std::string& name, QConfigPointer config) : JobConcept(name, config) {}
        virtual ~Concept() = default;

        virtual void run(const ContextPointer& jobContext) = 0;
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
        Varying& editInput() override { return _input; }

        template <class... A>
        Model(const std::string& name, const Varying& input, QConfigPointer config, A&&... args) :
            Concept(name, config),
            _data(Data(std::forward<A>(args)...)),
            _input(input),
            _output(Output(), name + ".o") {
            applyConfiguration();
        }

        template <class... A>
        static std::shared_ptr<Model> create(const std::string& name, const Varying& input, A&&... args) {
            assert(input.canCast<I>());
            return std::make_shared<Model>(name, input, std::make_shared<C>(), std::forward<A>(args)...);
        }

        void createConfiguration() {
            // A brand new config
            auto config = std::make_shared<C>();
            // Make sure we transfer the former children configs to the new config
            config->transferChildrenConfigs(Concept::_config);
            // swap
            Concept::_config = config;
            // Capture this
            Concept::_config->_jobConcept = this;
        }
        
        void applyConfiguration() override {
            TimeProfiler probe(("configure::" + JobConcept::getName()));

            jobConfigure(_data, *std::static_pointer_cast<C>(Concept::_config));
        }

        void run(const ContextPointer& jobContext) override {
            jobContext->jobConfig = std::static_pointer_cast<Config>(Concept::_config);
            if (jobContext->jobConfig->isEnabled()) {
                jobRun(_data, jobContext, _input.get<I>(), _output.edit<O>());
            }
            jobContext->jobConfig.reset();
        }
    };
    template <class T, class I, class C = Config> using ModelI = Model<T, C, I, None>;
    template <class T, class O, class C = Config> using ModelO = Model<T, C, None, O>;
    template <class T, class I, class O, class C = Config> using ModelIO = Model<T, C, I, O>;

    Job() {}
    Job(const ConceptPointer& concept) : _concept(concept) {}
    virtual ~Job() = default;

    const std::string& getName() const { return _concept->getName(); }
    const Varying getInput() const { return _concept->getInput(); }
    const Varying getOutput() const { return _concept->getOutput(); }

    QConfigPointer& getConfiguration() const { return _concept->getConfiguration(); }
    void applyConfiguration() { return _concept->applyConfiguration(); }

    template <class I> void feedInput(const I& in) { _concept->editInput().template edit<I>() = in; }
    template <class I, class S> void feedInput(int index, const S& inS) { (_concept->editInput().template editN<I>(index)).template edit<S>() = inS; }

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

    virtual void run(const ContextPointer& jobContext) {
        TimeProfiler probe(getName());
        auto startTime = std::chrono::high_resolution_clock::now();
        _concept->run(jobContext);
        _concept->setCPURunTime((std::chrono::high_resolution_clock::now() - startTime));
    }

protected:
    ConceptPointer _concept;
};


// A task is a specialized job to run a collection of other jobs
// It can be created on any type T by aliasing the type JobModel in the class T
// using JobModel = Task::Model<T>
// The class T is expected to have a "build" method acting as a constructor.
// The build method is where child Jobs can be added internally to the task
// where the input of the task can be setup to feed the child jobs
// and where the output of the task is defined
template <class JC, class TP>
class Task : public Job<JC, TP> {
public:
    using Context = JC;
    using TimeProfiler = TP;
    using ContextPointer = std::shared_ptr<Context>;
    using Config = JobConfig; //TaskConfig;
    using JobType = Job<JC, TP>;
    using None = typename JobType::None;
    using Concept = typename JobType::Concept;
    using ConceptPointer = typename JobType::ConceptPointer;
    using Jobs = std::vector<JobType>;

    Task(ConceptPointer concept) : JobType(concept) {}

    class TaskConcept : public Concept {
    public:
        Varying _input;
        Varying _output;
        Jobs _jobs;

        const Varying getInput() const override { return _input; }
        const Varying getOutput() const override { return _output; }
        Varying& editInput() override { return _input; }

        TaskConcept(const std::string& name, const Varying& input, QConfigPointer config) : Concept(name, config), _input(input) {config->_isTask = true;}

        // Create a new job in the container's queue; returns the job's output
        template <class NT, class... NA> const Varying addJob(std::string name, const Varying& input, NA&&... args) {
            _jobs.emplace_back((NT::JobModel::create(name, input, std::forward<NA>(args)...)));

            // Conect the child config to this task's config
            std::static_pointer_cast<JobConfig>(Concept::getConfiguration())->connectChildConfig(_jobs.back().getConfiguration(), name);

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

        TaskModel(const std::string& name, const Varying& input, QConfigPointer config) :
            TaskConcept(name, input, config),
            _data(Data()) {}

        template <class... A>
        static std::shared_ptr<TaskModel> create(const std::string& name, const Varying& input, A&&... args) {
            auto model = std::make_shared<TaskModel>(name, input, std::make_shared<C>());

            {
                TimeProfiler probe("build::" + model->getName());
                model->_data.build(*(model), model->_input, model->_output, std::forward<A>(args)...);
            }

            return model;
        }

        template <class... A>
        static std::shared_ptr<TaskModel> create(const std::string& name, A&&... args) {
            const auto input = Varying(Input());
            return create(name, input, std::forward<A>(args)...);
        }

        void createConfiguration() {
            // A brand new config
            auto config = std::make_shared<C>();
            // Make sure we transfer the former children configs to the new config
            config->transferChildrenConfigs(Concept::_config);
            // swap
            Concept::_config = config;
            // Capture this
            Concept::_config->_jobConcept = this;
        }

        QConfigPointer& getConfiguration() override {
            if (!Concept::_config) {
                createConfiguration();
            }
            return Concept::_config;
        }

        void applyConfiguration() override {
            TimeProfiler probe("configure::" + JobConcept::getName());
            jobConfigure(_data, *std::static_pointer_cast<C>(Concept::_config));
            for (auto& job : TaskConcept::_jobs) {
                job.applyConfiguration();
            }
        }

        void run(const ContextPointer& jobContext) override {
            auto config = std::static_pointer_cast<C>(Concept::_config);
            if (config->isEnabled()) {
                for (auto job : TaskConcept::_jobs) {
                    job.run(jobContext);
                    if (jobContext->taskFlow.doAbortTask()) {
                        jobContext->taskFlow.reset();
                        return;
                    }
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
};


// A Switch is a specialized job to run a collection of other jobs and switch between different branches at run time
// It can be created on any type T by aliasing the type JobModel in the class T
// using JobModel = Switch::Model<T>
// The class T is expected to have a "build" method acting as a constructor.
// The build method is where child Jobs can be added internally to the branches of the switch
// where the input of the switch can be setup to feed the child jobs
// and where the output of the switch is defined
template <class JC, class TP>
class Switch : public Job<JC, TP> {
public:
    using Context = JC;
    using TimeProfiler = TP;
    using ContextPointer = std::shared_ptr<Context>;
    using Config = JobConfig; //SwitchConfig;
    using JobType = Job<JC, TP>;
    using None = typename JobType::None;
    using Concept = typename JobType::Concept;
    using ConceptPointer = typename JobType::ConceptPointer;
    using Branches = std::unordered_map<uint8_t, JobType>;

    Switch(ConceptPointer concept) : JobType(concept) {}

    class SwitchConcept : public Concept {
    public:
        Varying _input;
        Varying _output;
        Branches _branches;

        const Varying getInput() const override { return _input; }
        const Varying getOutput() const override { return _output; }
        Varying& editInput() override { return _input; }

        SwitchConcept(const std::string& name, const Varying& input, QConfigPointer config) : Concept(name, config), _input(input)
        {config->_isTask = true; config->_isSwitch = true; }

        template <class NT, class... NA> const Varying addBranch(std::string name, uint8_t index, const Varying& input, NA&&... args) {
            auto& branch = _branches[index];
            branch = JobType(NT::JobModel::create(name, input, std::forward<NA>(args)...));

            // Conect the child config to this task's config
            std::static_pointer_cast<JobConfig>(Concept::getConfiguration())->connectChildConfig(branch.getConfiguration(), name);

            return branch.getOutput();
        }
        template <class NT, class... NA> const Varying addBranch(std::string name, uint8_t index, NA&&... args) {
            const auto input = Varying(typename NT::JobModel::Input());
            return addBranch<NT>(name, index, input, std::forward<NA>(args)...);
        }
    };

    template <class T, class C = Config, class I = None, class O = None> class SwitchModel : public SwitchConcept {
    public:
        using Data = T;
        using Input = I;
        using Output = O;

        Data _data;

        SwitchModel(const std::string& name, const Varying& input, QConfigPointer config) :
            SwitchConcept(name, input, config),
            _data(Data()) {
        }

        template <class... A>
        static std::shared_ptr<SwitchModel> create(const std::string& name, const Varying& input, A&&... args) {
            auto model = std::make_shared<SwitchModel>(name, input, std::make_shared<C>());

            {
                TimeProfiler probe("build::" + model->getName());
                model->_data.build(*(model), model->_input, model->_output, std::forward<A>(args)...);
            }

            return model;
        }

        template <class... A>
        static std::shared_ptr<SwitchModel> create(const std::string& name, A&&... args) {
            const auto input = Varying(Input());
            return create(name, input, std::forward<A>(args)...);
        }

        void createConfiguration() {
            // A brand new config
            auto config = std::make_shared<C>();
            // Make sure we transfer the former children configs to the new config
            config->transferChildrenConfigs(Concept::_config);
            // swap
            Concept::_config = config;
            // Capture this
            Concept::_config->_jobConcept = this;
        }

        QConfigPointer& getConfiguration() override {
            if (!Concept::_config) {
                createConfiguration();
            }
            return Concept::_config;
        }

        void applyConfiguration() override {
            TimeProfiler probe("configure::" + JobConcept::getName());
            jobConfigure(_data, *std::static_pointer_cast<C>(Concept::_config));
            for (auto& branch : SwitchConcept::_branches) {
                branch.second.applyConfiguration();
            }
        }

        void run(const ContextPointer& jobContext) override {
            auto config = std::static_pointer_cast<C>(Concept::_config);
            if (config->isEnabled()) {
                auto jobsIt = SwitchConcept::_branches.find(config->getBranch());
                if (jobsIt != SwitchConcept::_branches.end()) {
                    jobsIt->second.run(jobContext);
                }
            }
        }
    };
    template <class T, class C = Config> using Model = SwitchModel<T, C, None, None>;
    template <class T, class I, class C = Config> using ModelI = SwitchModel<T, C, I, None>;
    // TODO: Switches don't support Outputs yet
    //template <class T, class O, class C = SwitchConfig> using ModelO = SwitchModel<T, C, None, O>;
    //template <class T, class I, class O, class C = SwitchConfig> using ModelIO = SwitchModel<T, C, I, O>;

    // Create a new job in the Switches' branches; returns the job's output
    template <class T, class... A> const Varying addBranch(std::string name, uint8_t index, const Varying& input, A&&... args) {
        return std::static_pointer_cast<SwitchConcept>(JobType::_concept)->template addBranch<T>(name, index, input, std::forward<A>(args)...);
    }
    template <class T, class... A> const Varying addBranch(std::string name, uint8_t index, A&&... args) {
        const auto input = Varying(typename T::JobModel::Input());
        return std::static_pointer_cast<SwitchConcept>(JobType::_concept)->template addBranch<T>(name, index, input, std::forward<A>(args)...);
    }

    std::shared_ptr<Config> getConfiguration() {
        return std::static_pointer_cast<Config>(JobType::_concept->getConfiguration());
    }
};

template <class JC, class TP>
class Engine : public Task<JC, TP> {
public:
    using Context = JC;
    using ContextPointer = std::shared_ptr<Context>;
    using Config = JobConfig; //TaskConfig;

    using TaskType = Task<JC, TP>;
    using ConceptPointer = typename TaskType::ConceptPointer;

    Engine(const ConceptPointer& concept, const ContextPointer& context) : TaskType(concept), _context(context) {}
    ~Engine() = default;

    void reset(const ContextPointer& context) { _context = context; }

    void run() {
        if (_context) {
            run(_context);
        }
    }

protected:
    void run(const ContextPointer& jobContext) override {
        TaskType::run(_context);
    }

    ContextPointer _context;
};

}


#define Task_DeclareTypeAliases(ContextType, TimeProfiler) \
    using JobConfig = task::JobConfig; \
    using TaskConfig = task::JobConfig; \
    using SwitchConfig = task::JobConfig; \
    template <class T> using PersistentConfig = task::PersistentConfig<T>; \
    using Job = task::Job<ContextType, TimeProfiler>; \
    using Switch = task::Switch<ContextType, TimeProfiler>; \
    using Task = task::Task<ContextType, TimeProfiler>; \
    using Engine = task::Engine<ContextType, TimeProfiler>; \
    using Varying = task::Varying; \
    template < typename T0, typename T1 > using VaryingSet2 = task::VaryingSet2<T0, T1>; \
    template < typename T0, typename T1, typename T2 > using VaryingSet3 = task::VaryingSet3<T0, T1, T2>; \
    template < typename T0, typename T1, typename T2, typename T3 > using VaryingSet4 = task::VaryingSet4<T0, T1, T2, T3>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4 > using VaryingSet5 = task::VaryingSet5<T0, T1, T2, T3, T4>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5 > using VaryingSet6 = task::VaryingSet6<T0, T1, T2, T3, T4, T5>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6 > using VaryingSet7 = task::VaryingSet7<T0, T1, T2, T3, T4, T5, T6>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7 > using VaryingSet8 = task::VaryingSet8<T0, T1, T2, T3, T4, T5, T6, T7>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8 > using VaryingSet9 = task::VaryingSet9<T0, T1, T2, T3, T4, T5, T6, T7, T8>; \
    template < typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9 > using VaryingSet10 = task::VaryingSet10<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>; \
    template < class T, int NUM > using VaryingArray = task::VaryingArray<T, NUM>;



#include <Profile.h>
#include <PerfStat.h>

#define Task_DeclareCategoryTimeProfilerClass(className, category) \
    class className : public PerformanceTimer { \
    public: \
        className(const std::string& label) : PerformanceTimer(label.c_str()), profileRange(category(), label.c_str()) {} \
        Duration profileRange; \
    };

#endif // hifi_task_Task_h
