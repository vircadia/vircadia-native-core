//
//  Task.h
//  render/src/render
//
//  Created by Zach Pomerantz on 1/6/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Task_h
#define hifi_render_Task_h
#include <tuple>

#include <QtCore/qobject.h>

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <shared/JSONHelpers.h>

#include "SettingHandle.h"

#include "Context.h"
#include "Logging.h"

#include "gpu/Batch.h"
#include <PerfStat.h>

namespace render {

class Varying;


// A varying piece of data, to be used as Job/Task I/O
// TODO: Task IO
class Varying {
public:
    Varying() {}
    Varying(const Varying& var) : _concept(var._concept) {}
    Varying& operator=(const Varying& var) {
        _concept = var._concept;
        return (*this);
    }
    template <class T> Varying(const T& data) : _concept(std::make_shared<Model<T>>(data)) {}

    template <class T> bool canCast() const { return !!std::dynamic_pointer_cast<Model<T>>(_concept); }
    template <class T> const T& get() const { return std::static_pointer_cast<const Model<T>>(_concept)->_data; }
    template <class T> T& edit() { return std::static_pointer_cast<Model<T>>(_concept)->_data; }


    // access potential sub varyings contained in this one.
    Varying operator[] (uint8_t index) const { return (*_concept)[index]; }
    uint8_t length() const { return (*_concept).length(); }

    template <class T> Varying getN (uint8_t index) const { return get<T>()[index]; }
    template <class T> Varying editN (uint8_t index) { return edit<T>()[index]; }

protected:
    class Concept {
    public:
        virtual ~Concept() = default;

        virtual Varying operator[] (uint8_t index) const = 0;
        virtual uint8_t length() const = 0;
    };
    template <class T> class Model : public Concept {
    public:
        using Data = T;

        Model(const Data& data) : _data(data) {}
        virtual ~Model() = default;

        virtual Varying operator[] (uint8_t index) const override {
            Varying var;
            return var;
        }
        virtual uint8_t length() const override { return 0; }

        Data _data;
    };

    std::shared_ptr<Concept> _concept;
};

using VaryingPairBase = std::pair<Varying, Varying>;
template < typename T0, typename T1 >
class VaryingSet2 : public VaryingPairBase {
public:
    using Parent = VaryingPairBase;
    typedef void is_proxy_tag;

    VaryingSet2() : Parent(Varying(T0()), Varying(T1())) {}
    VaryingSet2(const VaryingSet2& pair) : Parent(pair.first, pair.second) {}
    VaryingSet2(const Varying& first, const Varying& second) : Parent(first, second) {}

    const T0& get0() const { return first.get<T0>(); }
    T0& edit0() { return first.edit<T0>(); }

    const T1& get1() const { return second.get<T1>(); }
    T1& edit1() { return second.edit<T1>(); }

    virtual Varying operator[] (uint8_t index) const {
        if (index == 1) {
            return std::get<1>((*this));
        } else {
            return std::get<0>((*this));
        }
    }
    virtual uint8_t length() const { return 2; }

    Varying hasVarying() const { return Varying((*this)); }
};


template <class T0, class T1, class T2>
class VaryingSet3 : public std::tuple<Varying, Varying,Varying>{
public:
    using Parent = std::tuple<Varying, Varying, Varying>;

    VaryingSet3() : Parent(Varying(T0()), Varying(T1()), Varying(T2())) {}
    VaryingSet3(const VaryingSet3& src) : Parent(std::get<0>(src), std::get<1>(src), std::get<2>(src)) {}
    VaryingSet3(const Varying& first, const Varying& second, const Varying& third) : Parent(first, second, third) {}

    const T0& get0() const { return std::get<0>((*this)).template get<T0>(); }
    T0& edit0() { return std::get<0>((*this)).template edit<T0>(); }

    const T1& get1() const { return std::get<1>((*this)).template get<T1>(); }
    T1& edit1() { return std::get<1>((*this)).template edit<T1>(); }

    const T2& get2() const { return std::get<2>((*this)).template get<T2>(); }
    T2& edit2() { return std::get<2>((*this)).template edit<T2>(); }

    virtual Varying operator[] (uint8_t index) const {
        if (index == 2) {
            return std::get<2>((*this));
        } else if (index == 1) {
            return std::get<1>((*this));
        } else {
            return std::get<0>((*this));
        }
    }
    virtual uint8_t length() const { return 3; }

    Varying hasVarying() const { return Varying((*this)); }
};

template <class T0, class T1, class T2, class T3>
class VaryingSet4 : public std::tuple<Varying, Varying, Varying, Varying>{
public:
    using Parent = std::tuple<Varying, Varying, Varying, Varying>;

    VaryingSet4() : Parent(Varying(T0()), Varying(T1()), Varying(T2()), Varying(T3())) {}
    VaryingSet4(const VaryingSet4& src) : Parent(std::get<0>(src), std::get<1>(src), std::get<2>(src), std::get<3>(src)) {}
    VaryingSet4(const Varying& first, const Varying& second, const Varying& third, const Varying& fourth) : Parent(first, second, third, fourth) {}

    const T0& get0() const { return std::get<0>((*this)).template get<T0>(); }
    T0& edit0() { return std::get<0>((*this)).template edit<T0>(); }

    const T1& get1() const { return std::get<1>((*this)).template get<T1>(); }
    T1& edit1() { return std::get<1>((*this)).template edit<T1>(); }

    const T2& get2() const { return std::get<2>((*this)).template get<T2>(); }
    T2& edit2() { return std::get<2>((*this)).template edit<T2>(); }

    const T3& get3() const { return std::get<3>((*this)).template get<T3>(); }
    T3& edit3() { return std::get<3>((*this)).template edit<T3>(); }

    virtual Varying operator[] (uint8_t index) const {
        if (index == 3) {
            return std::get<3>((*this));
        } else if (index == 2) {
            return std::get<2>((*this));
        } else if (index == 1) {
            return std::get<1>((*this));
        } else {
            return std::get<0>((*this));
        }
    }
    virtual uint8_t length() const { return 4; }

    Varying hasVarying() const { return Varying((*this)); }
};


template <class T0, class T1, class T2, class T3, class T4>
class VaryingSet5 : public std::tuple<Varying, Varying, Varying, Varying, Varying>{
public:
    using Parent = std::tuple<Varying, Varying, Varying, Varying, Varying>;

    VaryingSet5() : Parent(Varying(T0()), Varying(T1()), Varying(T2()), Varying(T3()), Varying(T4())) {}
    VaryingSet5(const VaryingSet5& src) : Parent(std::get<0>(src), std::get<1>(src), std::get<2>(src), std::get<3>(src), std::get<4>(src)) {}
    VaryingSet5(const Varying& first, const Varying& second, const Varying& third, const Varying& fourth, const Varying& fifth) : Parent(first, second, third, fourth, fifth) {}

    const T0& get0() const { return std::get<0>((*this)).template get<T0>(); }
    T0& edit0() { return std::get<0>((*this)).template edit<T0>(); }

    const T1& get1() const { return std::get<1>((*this)).template get<T1>(); }
    T1& edit1() { return std::get<1>((*this)).template edit<T1>(); }

    const T2& get2() const { return std::get<2>((*this)).template get<T2>(); }
    T2& edit2() { return std::get<2>((*this)).template edit<T2>(); }

    const T3& get3() const { return std::get<3>((*this)).template get<T3>(); }
    T3& edit3() { return std::get<3>((*this)).template edit<T3>(); }

    const T4& get4() const { return std::get<4>((*this)).template get<T4>(); }
    T4& edit4() { return std::get<4>((*this)).template edit<T4>(); }

    virtual Varying operator[] (uint8_t index) const {
        if (index == 4) {
            return std::get<4>((*this));
        } else if (index == 3) {
            return std::get<3>((*this));
        } else if (index == 2) {
            return std::get<2>((*this));
        } else if (index == 1) {
            return std::get<1>((*this));
        } else {
            return std::get<0>((*this));
        }
    }
    virtual uint8_t length() const { return 5; }

    Varying hasVarying() const { return Varying((*this)); }
};

template <class T0, class T1, class T2, class T3, class T4, class T5>
class VaryingSet6 : public std::tuple<Varying, Varying, Varying, Varying, Varying, Varying>{
public:
    using Parent = std::tuple<Varying, Varying, Varying, Varying, Varying, Varying>;

    VaryingSet6() : Parent(Varying(T0()), Varying(T1()), Varying(T2()), Varying(T3()), Varying(T4()), Varying(T5())) {}
    VaryingSet6(const VaryingSet6& src) : Parent(std::get<0>(src), std::get<1>(src), std::get<2>(src), std::get<3>(src), std::get<4>(src), std::get<5>(src)) {}
    VaryingSet6(const Varying& first, const Varying& second, const Varying& third, const Varying& fourth, const Varying& fifth, const Varying& sixth) : Parent(first, second, third, fourth, fifth, sixth) {}

    const T0& get0() const { return std::get<0>((*this)).template get<T0>(); }
    T0& edit0() { return std::get<0>((*this)).template edit<T0>(); }

    const T1& get1() const { return std::get<1>((*this)).template get<T1>(); }
    T1& edit1() { return std::get<1>((*this)).template edit<T1>(); }

    const T2& get2() const { return std::get<2>((*this)).template get<T2>(); }
    T2& edit2() { return std::get<2>((*this)).template edit<T2>(); }

    const T3& get3() const { return std::get<3>((*this)).template get<T3>(); }
    T3& edit3() { return std::get<3>((*this)).template edit<T3>(); }

    const T4& get4() const { return std::get<4>((*this)).template get<T4>(); }
    T4& edit4() { return std::get<4>((*this)).template edit<T4>(); }

    const T5& get5() const { return std::get<5>((*this)).template get<T5>(); }
    T5& edit5() { return std::get<5>((*this)).template edit<T5>(); }

    Varying hasVarying() const { return Varying((*this)); }
};

template <class T0, class T1, class T2, class T3, class T4, class T5, class T6>
class VaryingSet7 : public std::tuple<Varying, Varying, Varying, Varying, Varying, Varying, Varying>{
public:
    using Parent = std::tuple<Varying, Varying, Varying, Varying, Varying, Varying, Varying>;
    
    VaryingSet7() : Parent(Varying(T0()), Varying(T1()), Varying(T2()), Varying(T3()), Varying(T4()), Varying(T5()), Varying(T6())) {}
    VaryingSet7(const VaryingSet7& src) : Parent(std::get<0>(src), std::get<1>(src), std::get<2>(src), std::get<3>(src), std::get<4>(src), std::get<5>(src), std::get<6>(src)) {}
    VaryingSet7(const Varying& first, const Varying& second, const Varying& third, const Varying& fourth, const Varying& fifth, const Varying& sixth, const Varying& seventh) : Parent(first, second, third, fourth, fifth, sixth, seventh) {}
    
    const T0& get0() const { return std::get<0>((*this)).template get<T0>(); }
    T0& edit0() { return std::get<0>((*this)).template edit<T0>(); }
    
    const T1& get1() const { return std::get<1>((*this)).template get<T1>(); }
    T1& edit1() { return std::get<1>((*this)).template edit<T1>(); }
    
    const T2& get2() const { return std::get<2>((*this)).template get<T2>(); }
    T2& edit2() { return std::get<2>((*this)).template edit<T2>(); }
    
    const T3& get3() const { return std::get<3>((*this)).template get<T3>(); }
    T3& edit3() { return std::get<3>((*this)).template edit<T3>(); }
    
    const T4& get4() const { return std::get<4>((*this)).template get<T4>(); }
    T4& edit4() { return std::get<4>((*this)).template edit<T4>(); }
    
    const T5& get5() const { return std::get<5>((*this)).template get<T5>(); }
    T5& edit5() { return std::get<5>((*this)).template edit<T5>(); }
    
    const T6& get6() const { return std::get<6>((*this)).template get<T6>(); }
    T6& edit6() { return std::get<6>((*this)).template edit<T6>(); }
    
    Varying hasVarying() const { return Varying((*this)); }
};

    
template < class T, int NUM >
class VaryingArray : public std::array<Varying, NUM> {
public:
    VaryingArray() {
        for (size_t i = 0; i < NUM; i++) {
            (*this)[i] = Varying(T());
        }
    }
};

class Job;
class JobConcept;
class Task;
class JobNoIO {};

template <class C> class PersistentConfig : public C {
public:
    const QString DEFAULT = "Default";
    const QString NONE = "None";

    PersistentConfig() = delete;
    PersistentConfig(const QString& path) :
        _preset(QStringList() << "Render" << "Engine" << path, DEFAULT) { }
    PersistentConfig(const QStringList& path) :
        _preset(QStringList() << "Render" << "Engine" << path, DEFAULT) { }
    PersistentConfig(const QString& path, bool enabled) : C(enabled),
        _preset(QStringList() << "Render" << "Engine" << path, enabled ? DEFAULT : NONE) { }
    PersistentConfig(const QStringList& path, bool enabled) : C(enabled),
        _preset(QStringList() << "Render" << "Engine" << path, enabled ? DEFAULT : NONE) { }

    QStringList getPresetList() {
        if (_presets.empty()) {
            setPresetList(QJsonObject());
        }
        return _presets.keys();
    }

    virtual void setPresetList(const QJsonObject& list) override {
        assert(_presets.empty());

        _default = toJsonValue(*this).toObject().toVariantMap();

        _presets.unite(list.toVariantMap());
        if (C::alwaysEnabled || C::enabled) {
            _presets.insert(DEFAULT, _default);
        }
        if (!C::alwaysEnabled) {
            _presets.insert(NONE, QVariantMap{{ "enabled", false }});
        }

        auto preset = _preset.get();
        if (preset != _preset.getDefault() && _presets.contains(preset)) {
            // Load the persisted configuration
            C::load(_presets[preset].toMap());
        }
    }

    QString getPreset() { return _preset.get(); }

    void setPreset(const QString& preset) {
        _preset.set(preset);
        if (_presets.contains(preset)) {
            // Always start back at default to remain deterministic
            QVariantMap config = _default;
            QVariantMap presetConfig = _presets[preset].toMap();
            for (auto it = presetConfig.cbegin(); it != presetConfig.cend(); it++) {
                config.insert(it.key(), it.value());
            }
            C::load(config);
        }
    }

protected:
    QVariantMap _default;
    QVariantMap _presets;
    Setting::Handle<QString> _preset;
};

// A default Config is always on; to create an enableable Config, use the ctor JobConfig(bool enabled)
class JobConfig : public QObject {
    Q_OBJECT
    Q_PROPERTY(double cpuRunTime READ getCPURunTime NOTIFY newStats()) //ms
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)

    double _msCPURunTime{ 0.0 };
public:
    using Persistent = PersistentConfig<JobConfig>;

    JobConfig() = default;
    JobConfig(bool enabled) : alwaysEnabled{ false }, enabled{ enabled } {}

    bool isEnabled() { return alwaysEnabled || enabled; }
    void setEnabled(bool enable) { enabled = alwaysEnabled || enable; }

    bool alwaysEnabled{ true };
    bool enabled{ true };

    virtual void setPresetList(const QJsonObject& object) {
        for (auto it = object.begin(); it != object.end(); it++) {
            JobConfig* child = findChild<JobConfig*>(it.key(), Qt::FindDirectChildrenOnly);
            if (child) {
                child->setPresetList(it.value().toObject());
            }
        }
    }

    // This must be named toJSON to integrate with the global scripting JSON object
    Q_INVOKABLE QString toJSON() { return QJsonDocument(toJsonValue(*this).toObject()).toJson(QJsonDocument::Compact); }
    Q_INVOKABLE void load(const QVariantMap& map) { qObjectFromJsonValue(QJsonObject::fromVariantMap(map), *this); emit loaded(); }

    // Running Time measurement
    // The new stats signal is emitted once per run time of a job when stats  (cpu runtime) are updated
    void setCPURunTime(double mstime) { _msCPURunTime = mstime; emit newStats(); }
    double getCPURunTime() const { return _msCPURunTime; }

public slots:
    void load(const QJsonObject& val) { qObjectFromJsonValue(val, *this); emit loaded(); }

signals:
    void loaded();
    void newStats();
};

class TaskConfig : public JobConfig {
    Q_OBJECT
public:
    using QConfigPointer = std::shared_ptr<QObject>;

    using Persistent = PersistentConfig<TaskConfig>;

    TaskConfig() = default ;
    TaskConfig(bool enabled) : JobConfig(enabled) {}

    // getter for qml integration, prefer the templated getter
    Q_INVOKABLE QObject* getConfig(const QString& name) { return QObject::findChild<JobConfig*>(name); }
    // getter for cpp (strictly typed), prefer this getter
    template <class T> typename T::Config* getConfig(std::string job = "") const {
        QString name = job.empty() ? QString() : QString(job.c_str()); // an empty string is not a null string
        return findChild<typename T::Config*>(name);
    }

    void connectChildConfig(QConfigPointer childConfig, const std::string& name);
    void transferChildrenConfigs(QConfigPointer source);

public slots:
    void refresh();

private:
    friend Task;
    JobConcept* _task;
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
template <class T> void jobRun(T& data, const RenderContextPointer& renderContext, const JobNoIO& input, JobNoIO& output) {
    data.run(renderContext);
}
template <class T, class I> void jobRun(T& data, const RenderContextPointer& renderContext, const I& input, JobNoIO& output) {
    data.run(renderContext, input);
}
template <class T, class O> void jobRun(T& data, const RenderContextPointer& renderContext, const JobNoIO& input, O& output) {
    data.run(renderContext, output);
}
template <class T, class I, class O> void jobRun(T& data, const RenderContextPointer& renderContext, const I& input, O& output) {
    data.run(renderContext, input, output);
}

// The guts of a job
class JobConcept {
public:
    using Config = JobConfig;
    using QConfigPointer = std::shared_ptr<QObject>;

    JobConcept(QConfigPointer config) : _config(config) {}
    virtual ~JobConcept() = default;

    virtual const Varying getInput() const { return Varying(); }
    virtual const Varying getOutput() const { return Varying(); }

    virtual QConfigPointer& getConfiguration() { return _config; }
    virtual void applyConfiguration() = 0;

    virtual void run(const RenderContextPointer& renderContext) = 0;

protected:
    void setCPURunTime(double mstime) { std::static_pointer_cast<Config>(_config)->setCPURunTime(mstime); }

    QConfigPointer _config;

    friend class Job;
};

class Job {
public:
    using Concept = JobConcept;
    using Config = JobConfig;
    using QConfigPointer = std::shared_ptr<QObject>;
    using None = JobNoIO;
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
            jobConfigure(_data, *std::static_pointer_cast<C>(_config));
        }

        void run(const RenderContextPointer& renderContext) override {
            renderContext->jobConfig = std::static_pointer_cast<Config>(_config);
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

    void run(const RenderContextPointer& renderContext) {
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
class Task : public Job {
public:
    using Config = TaskConfig;
    using QConfigPointer = Job::QConfigPointer;
    using None = Job::None;
    using Concept = Job::Concept;
    using Jobs = std::vector<Job>;

    Task(std::string name, ConceptPointer concept) : Job(name, concept) {}

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
            std::static_pointer_cast<TaskConfig>(getConfiguration())->connectChildConfig(_jobs.back().getConfiguration(), name);

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
          //  std::static_pointer_cast<C>(model->_config)->_task = model.get();

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
            config->transferChildrenConfigs(_config);
            // swap
            _config = config;
            // Capture this
            std::static_pointer_cast<C>(_config)->_task = this;
        }

        QConfigPointer& getConfiguration() override {
            if (!_config) {
                createConfiguration();
            }
            return _config;
        }

        void applyConfiguration() override {
            jobConfigure(_data, *std::static_pointer_cast<C>(_config));
            for (auto& job : _jobs) {
                job.applyConfiguration();
            }
        }

        void run(const RenderContextPointer& renderContext) override {
            auto config = std::static_pointer_cast<C>(_config);
            if (config->alwaysEnabled || config->enabled) {
                for (auto job : _jobs) {
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
        return std::static_pointer_cast<TaskConcept>( _concept)->addJob<T>(name, input, std::forward<A>(args)...);
    }
    template <class T, class... A> const Varying addJob(std::string name, A&&... args) {
        const auto input = Varying(typename T::JobModel::Input());
        return std::static_pointer_cast<TaskConcept>( _concept)->addJob<T>(name, input, std::forward<A>(args)...);
    }

    std::shared_ptr<Config> getConfiguration() {
        return std::static_pointer_cast<Config>(_concept->getConfiguration());
    }

protected:
};

// Versions of the COnfig integrating a gpu & batch timer
class GPUJobConfig : public JobConfig {
    Q_OBJECT
    Q_PROPERTY(double gpuRunTime READ getGPURunTime)
    Q_PROPERTY(double batchRunTime READ getBatchRunTime)

    double _msGPURunTime { 0.0 };
    double _msBatchRunTime { 0.0 };
public:
    using Persistent = PersistentConfig<GPUJobConfig>;

    GPUJobConfig() = default;
    GPUJobConfig(bool enabled) : JobConfig(enabled) {}

    // Running Time measurement on GPU and for Batch execution
    void setGPUBatchRunTime(double msGpuTime, double msBatchTime) { _msGPURunTime = msGpuTime; _msBatchRunTime = msBatchTime; }
    double getGPURunTime() const { return _msGPURunTime; }
    double getBatchRunTime() const { return _msBatchRunTime; }
};

class GPUTaskConfig : public TaskConfig {
    Q_OBJECT
    Q_PROPERTY(double gpuRunTime READ getGPURunTime)
    Q_PROPERTY(double batchRunTime READ getBatchRunTime)

    double _msGPURunTime { 0.0 };
    double _msBatchRunTime { 0.0 };
public:

    using Persistent = PersistentConfig<GPUTaskConfig>;


    GPUTaskConfig() = default;
    GPUTaskConfig(bool enabled) : TaskConfig(enabled) {}

    // Running Time measurement on GPU and for Batch execution
    void setGPUBatchRunTime(double msGpuTime, double msBatchTime) { _msGPURunTime = msGpuTime; _msBatchRunTime = msBatchTime; }
    double getGPURunTime() const { return _msGPURunTime; }
    double getBatchRunTime() const { return _msBatchRunTime; }
};

}

#endif // hifi_render_Task_h
