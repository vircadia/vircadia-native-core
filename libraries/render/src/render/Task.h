//
//  Task.h
//  render/src/render
//
//  Created by Zach Pomerantz on 1/6/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Task_h
#define hifi_render_Task_h

#include <qscriptengine.h> // QObject

#include "Context.h"

#include "gpu/Batch.h"
#include <PerfStat.h>

namespace render {

// A varying piece of data, to be used as Job/Task I/O
// TODO: Task IO
class Varying {
public:
    Varying() {}
    Varying(const Varying& var) : _concept(var._concept) {}
    template <class T> Varying(const T& data) : _concept(std::make_shared<Model<T>>(data)) {}

    template <class T> T& edit() { return std::static_pointer_cast<Model<T>>(_concept)->_data; }
    template <class T> const T& get() { return std::static_pointer_cast<const Model<T>>(_concept)->_data; }

protected:
    class Concept {
    public:
        virtual ~Concept() = default;
    };
    template <class T> class Model : public Concept {
    public:
        using Data = T;

        Model(const Data& data) : _data(data) {}
        virtual ~Model() = default;

        Data _data;
    };

    std::shared_ptr<Concept> _concept;
};

class NoConfiguration : public QObject {
    Q_OBJECT
};

template <class T, class C> void jobConfigure(T& model, const C& configuration) {
    model.configure(configuration);
}
template<class T> void jobConfigure(T&, const NoConfiguration&) {
}

// FIXME: In c++17, use default classes of nullptr_t to combine these
template <class T> void jobRun(T& model, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    model.run(sceneContext, renderContext);
}
template <class T, class I> void jobRunI(T& model, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input) {
    model.run(sceneContext, renderContext, input);
}
template <class T, class O> void jobRunO(T& model, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, O& output) {
    model.run(sceneContext, renderContext, output);
}
template <class T, class I, class O> void jobRunIO(T& model, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input, O& output) {
    model.run(sceneContext, renderContext, input, output);
}

class Job {
public:
    using QObjectPointer = std::shared_ptr<QObject>;

    // The guts of a job
    class Concept {
    public:
        Concept() = default;
        virtual ~Concept() = default;

        bool isEnabled() const { return _isEnabled; }
        void setEnabled(bool isEnabled) { _isEnabled = isEnabled; }

        virtual const Varying getInput() const { return Varying(); }
        virtual const Varying getOutput() const { return Varying(); }
        virtual const QObjectPointer getConfiguration() = 0;
        virtual void setConfiguration(const QObjectPointer& configuration) = 0;
        virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) = 0;

    protected:
        bool _isEnabled = true;
    };
    using ConceptPointer = std::shared_ptr<Concept>;

    template <class T, class C = NoConfiguration> class Model : public Concept {
    public:
        using Data = T;
        using ConfigurationPointer = std::shared_ptr<C>;

        Data _data;
        ConfigurationPointer _configuration;

        Model() : _configuration{ std::make_shared<C>() } {}
        Model(Data data) : _data(data), _configuration{ std::make_shared<C>() } {
            jobConfigure(_data, *_configuration);
        }

        const QObjectPointer getConfiguration() { return _configuration; }
        void setConfiguration(const QObjectPointer& configuration) {
            _configuration = std::static_pointer_cast<C>(configuration);
            jobConfigure(_data, *_configuration);
        }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
            if (isEnabled()) {
                jobRun(_data, sceneContext, renderContext);
            }
        }
    };

    template <class T, class I, class C = NoConfiguration> class ModelI : public Concept {
    public:
        using Data = T;
        using ConfigurationPointer = std::shared_ptr<C>;
        using Input = I;

        Data _data;
        ConfigurationPointer _configuration;
        Varying _input;

        const Varying getInput() const { return _input; }

        ModelI(const Varying& input, Data data = Data()) : _data(data), _input(input) {}
        ModelI(Data data) : _data(data) {}

        const QObjectPointer getConfiguration() { return _configuration; }
        void setConfiguration(const QObjectPointer& configuration) {
            _configuration = std::static_pointer_cast<C>(configuration);
            jobConfigure(_data, *_configuration);
        }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
            if (isEnabled()) {
                jobRunI(_data, sceneContext, renderContext, _input.get<I>());
            }
        }
    };

    template <class T, class O, class C = NoConfiguration> class ModelO : public Concept {
    public:
        using Data = T;
        using ConfigurationPointer = std::shared_ptr<C>;
        using Output = O;

        Data _data;
        ConfigurationPointer _configuration;
        Varying _output;

        const Varying getOutput() const { return _output; }

        ModelO(Data data) : _data(data), _output(Output()) {}
        ModelO() : _output(Output()) {}

        const QObjectPointer getConfiguration() { return _configuration; }
        void setConfiguration(const QObjectPointer& configuration) {
            _configuration = std::static_pointer_cast<C>(configuration);
            jobConfigure(_data, *_configuration);
        }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
            if (isEnabled()) {
                jobRunO(_data, sceneContext, renderContext, _output.edit<O>());
            }
        }
    };

    template <class T, class I, class O, class C = NoConfiguration> class ModelIO : public Concept {
    public:
        using Data = T;
        using ConfigurationPointer = std::shared_ptr<C>;
        using Input = I;
        using Output = O;

        Data _data;
        ConfigurationPointer _configuration;
        Varying _input;
        Varying _output;

        const Varying getInput() const { return _input; }
        const Varying getOutput() const { return _output; }

        ModelIO(const Varying& input, Data data = Data()) : _data(data), _input(input), _output(Output()) {}
        ModelIO(Data data) : _data(data), _output(Output()) {}

        const QObjectPointer getConfiguration() { return _configuration; }
        void setConfiguration(const QObjectPointer& configuration) {
            _configuration = std::static_pointer_cast<C>(configuration);
            jobConfigure(_data, *_configuration);
        }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
            if (isEnabled()) {
                jobRunIO(_data, sceneContext, renderContext, _input.get<I>(), _output.edit<O>());
            }
        }
    };


    Job(std::string name, ConceptPointer concept) : _concept(concept), _name(name) {}

    bool isEnabled() const { return _concept->isEnabled(); }
    void setEnabled(bool isEnabled) { _concept->setEnabled(isEnabled); }

    const Varying getInput() const { return _concept->getInput(); }
    const Varying getOutput() const { return _concept->getOutput(); }
    const QObjectPointer getConfiguration() const { return _concept->getConfiguration(); }
    void setConfiguration(const QObjectPointer& configuration) { return _concept->setConfiguration(configuration); }

    template <class T> T& edit() {
        auto concept = std::static_pointer_cast<typename T::JobModel>(_concept);
        assert(concept);
        return concept->_data;
    }

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
        PerformanceTimer perfTimer(_name.c_str());
        PROFILE_RANGE(_name.c_str());

        _concept->run(sceneContext, renderContext);
    }

    class Container {
    public:
        using Job = Job;
        using Jobs = std::vector<Job>;

        Container() = default;
        virtual ~Container() = default;

        // Queue a new job to the container; returns the job's output
        template <class T, class... A> const Varying addJob(std::string name, A&&... args) {
            _jobs.emplace_back(name, std::make_shared<typename T::JobModel>(std::forward<A>(args)...));
            return _jobs.back().getOutput();
        }

        void enableJob(size_t jobIndex, bool enable = true) { _jobs.at(jobIndex).setEnabled(enable); }
        bool getEnableJob(size_t jobIndex) const { return _jobs.at(jobIndex).isEnabled(); }

    protected:
        Jobs _jobs;
    };

protected:
    friend class Container;

    ConceptPointer _concept;
    std::string _name = "";
};
// Define our vernacular: Engine->Tasks->Jobs
using Task = Job::Container;
using TaskPointer = std::shared_ptr<Task>;
using Tasks = std::vector<TaskPointer>;

}

#endif // hifi_render_Task_h
