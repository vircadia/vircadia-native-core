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

    template <class T> T& edit() { return std::static_pointer_cast<Model<T>>(_concept)->_data; }
    template <class T> const T& get() const { return std::static_pointer_cast<const Model<T>>(_concept)->_data; }

    
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
        
        virtual Varying operator[] (uint8_t index) const {
            Varying var;
            return var;
        }
        virtual uint8_t length() const { return 0; }

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
    Q_PROPERTY(quint64 cpuRunTime READ getCPUTRunTime NOTIFY newStats())

    quint64 _CPURunTime{ 0 };
public:
    using Persistent = PersistentConfig<JobConfig>;

    JobConfig() = default;
    JobConfig(bool enabled) : alwaysEnabled{ false }, enabled{ enabled } {}

    bool isEnabled() { return alwaysEnabled || enabled; }
    void setEnabled(bool enable) { enabled = enable; }

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
    void setCPURunTime(quint64 ustime) { _CPURunTime = ustime; emit newStats(); }
    quint64 getCPUTRunTime() const { return _CPURunTime; }

public slots:
    void load(const QJsonObject& val) { qObjectFromJsonValue(val, *this); emit loaded(); }

signals:
    void loaded();
    void newStats();
};

class TaskConfig : public JobConfig {
    Q_OBJECT
public:
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

public slots:
    void refresh();

private:
    friend class Task;
    Task* _task;
};

template <class T, class C> void jobConfigure(T& data, const C& configuration) {
    data.configure(configuration);
}
template<class T> void jobConfigure(T&, const JobConfig&) {
    // nop, as the default JobConfig was used, so the data does not need a configure method
}
template <class T> void jobRun(T& data, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const JobNoIO& input, JobNoIO& output) {
    data.run(sceneContext, renderContext);
}
template <class T, class I> void jobRun(T& data, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input, JobNoIO& output) {
    data.run(sceneContext, renderContext, input);
}
template <class T, class O> void jobRun(T& data, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const JobNoIO& input, O& output) {
    data.run(sceneContext, renderContext, output);
}
template <class T, class I, class O> void jobRun(T& data, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input, O& output) {
    data.run(sceneContext, renderContext, input, output);
}

class Job {
public:
    using Config = JobConfig;
    using QConfigPointer = std::shared_ptr<QObject>;
    using None = JobNoIO;

    // The guts of a job
    class Concept {
    public:
        Concept(QConfigPointer config) : _config(config) {}
        virtual ~Concept() = default;

        virtual const Varying getInput() const { return Varying(); }
        virtual const Varying getOutput() const { return Varying(); }

        virtual QConfigPointer& getConfiguration() { return _config; }
        virtual void applyConfiguration() = 0;

        virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) = 0;

    protected:
        void setCPURunTime(quint64 ustime) { std::static_pointer_cast<Config>(_config)->setCPURunTime(ustime); }

        QConfigPointer _config;

        friend class Job;
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

        const Varying getInput() const { return _input; }
        const Varying getOutput() const { return _output; }

        template <class... A>
        Model(const Varying& input, A&&... args) :
            Concept(std::make_shared<C>()), _data(Data(std::forward<A>(args)...)), _input(input), _output(Output()) {
            applyConfiguration();
        }

        void applyConfiguration() {
            jobConfigure(_data, *std::static_pointer_cast<C>(_config));
        }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
            renderContext->jobConfig = std::static_pointer_cast<Config>(_config);
            if (renderContext->jobConfig->alwaysEnabled || renderContext->jobConfig->isEnabled()) {
                jobRun(_data, sceneContext, renderContext, _input.get<I>(), _output.edit<O>());
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

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
        PerformanceTimer perfTimer(_name.c_str());
        PROFILE_RANGE(_name.c_str());
        auto start = usecTimestampNow();

        _concept->run(sceneContext, renderContext);

        _concept->setCPURunTime(usecTimestampNow() - start);
    }

    protected:
    ConceptPointer _concept;
    std::string _name = "";
};

// A task is a specialized job to run a collection of other jobs
// It is defined with JobModel = Task::Model<T>
class Task {
public:
    using Config = TaskConfig;
    using QConfigPointer = Job::QConfigPointer;
    using None = Job::None;

    template <class T, class C = Config, class I = None, class O = None> class Model : public Job::Concept {
    public:
        using Data = T;
        using Input = I;
        using Output = O;

        Data _data;
        Varying _input;
        Varying _output;

        const Varying getInput() const { return _input; }
        const Varying getOutput() const { return _output; }

        template <class... A>
        Model(const Varying& input, A&&... args) :
            Concept(nullptr), _data(Data(std::forward<A>(args)...)), _input(input), _output(Output()) {
            // Recreate the Config to use the templated type
            _data.template createConfiguration<C>();
            _config = _data.getConfiguration();
            applyConfiguration();
        }

        void applyConfiguration() {
            jobConfigure(_data, *std::static_pointer_cast<C>(_config));
        }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
            renderContext->jobConfig = std::static_pointer_cast<Config>(_config);
            if (renderContext->jobConfig->alwaysEnabled || renderContext->jobConfig->enabled) {
                jobRun(_data, sceneContext, renderContext, _input.get<I>(), _output.edit<O>());
            }
            renderContext->jobConfig.reset();
        }
    };
    template <class T, class I, class C = Config> using ModelI = Model<T, C, I, None>;
    template <class T, class O, class C = Config> using ModelO = Model<T, C, None, O>;
    template <class T, class I, class O, class C = Config> using ModelIO = Model<T, C, I, O>;

    using Jobs = std::vector<Job>;

    // Create a new job in the container's queue; returns the job's output
    template <class T, class... A> const Varying addJob(std::string name, const Varying& input, A&&... args) {
        _jobs.emplace_back(name, std::make_shared<typename T::JobModel>(input, std::forward<A>(args)...));
        QConfigPointer config = _jobs.back().getConfiguration();
        config->setParent(getConfiguration().get());
        config->setObjectName(name.c_str());

        // Connect loaded->refresh
        QObject::connect(config.get(), SIGNAL(loaded()), getConfiguration().get(), SLOT(refresh()));
        static const char* DIRTY_SIGNAL = "dirty()";
        if (config->metaObject()->indexOfSignal(DIRTY_SIGNAL) != -1) {
            // Connect dirty->refresh if defined
            QObject::connect(config.get(), SIGNAL(dirty()), getConfiguration().get(), SLOT(refresh()));
        }

        return _jobs.back().getOutput();
    }
    template <class T, class... A> const Varying addJob(std::string name, A&&... args) {
        const auto input = Varying(typename T::JobModel::Input());
        return addJob<T>(name, input, std::forward<A>(args)...);
    }

    template <class C> void createConfiguration() {
        auto config = std::make_shared<C>();
        if (_config) {
            // Transfer children to the new configuration
            auto children = _config->children();
            for (auto& child : children) {
                child->setParent(config.get());
                QObject::connect(child, SIGNAL(loaded()), config.get(), SLOT(refresh()));
                static const char* DIRTY_SIGNAL = "dirty()";
                if (child->metaObject()->indexOfSignal(DIRTY_SIGNAL) != -1) {
                    // Connect dirty->refresh if defined
                    QObject::connect(child, SIGNAL(dirty()), config.get(), SLOT(refresh()));
                }
            }
        }
        _config = config;
        std::static_pointer_cast<Config>(_config)->_task = this;
    }

    std::shared_ptr<Config> getConfiguration() {
        if (!_config) {
            createConfiguration<Config>();
        }
        return std::static_pointer_cast<Config>(_config);
    }

    void configure(const QObject& configuration) {
        for (auto& job : _jobs) {
            job.applyConfiguration();
            
        }
    }

protected:
    template <class T, class C, class I, class O> friend class Model;

    QConfigPointer _config;
    Jobs _jobs;
};

}


#endif // hifi_render_Task_h
