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

// FIXME: In c++17, use default classes of nullptr_t to combine these
template <class T> void jobRun(T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    jobModel.run(sceneContext, renderContext);
}
template <class T, class I> void jobRunI(T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input) {
    jobModel.run(sceneContext, renderContext, input);
}
template <class T, class O> void jobRunO(T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, O& output) {
    jobModel.run(sceneContext, renderContext, output);
}
template <class T, class I, class O> void jobRunIO(T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input, O& output) {
    jobModel.run(sceneContext, renderContext, input, output);
}

// The base class for a task that runs on the SceneContext
class Task {
public:
    // The guts of a task; tasks are composed of multiple Jobs that execute serially
    class Job {
    public:
        friend class Task;

        // The guts of a job; jobs are composed of a concept
        class Concept {
        public:
            Concept() = default;
            virtual ~Concept() = default;

            bool isEnabled() const { return _isEnabled; }
            void setEnabled(bool isEnabled) { _isEnabled = isEnabled; }

            virtual const Varying getInput() const { return Varying(); }
            virtual const Varying getOutput() const { return Varying(); }
            virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) = 0;

        protected:

            bool _isEnabled = true;
        };
        using ConceptPointer = std::shared_ptr<Concept>;


        template <class T> class Model : public Concept {
        public:
            typedef T Data;

            Data _data;

            Model() {}
            Model(Data data): _data(data) {}

            void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
                if (isEnabled()) {
                    jobRun(_data, sceneContext, renderContext);
                }
            }
        };

        template <class T, class I> class ModelI : public Concept {
        public:
            typedef T Data;
            typedef I Input;

            Data _data;
            Varying _input;

            const Varying getInput() const { return _input; }

            ModelI(const Varying& input, Data data = Data()) : _data(data), _input(input) {}
            ModelI(Data data) : _data(data) {}

            void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
                if (isEnabled()) {
                    jobRunI(_data, sceneContext, renderContext, _input.get<I>());
                }
            }
        };

        template <class T, class O> class ModelO : public Concept {
        public:
            typedef T Data;
            typedef O Output;

            Data _data;
            Varying _output;

            const Varying getOutput() const { return _output; }

            ModelO(Data data) : _data(data), _output(Output()) {}
            ModelO() : _output(Output()) {}

            void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
                if (isEnabled()) {
                    jobRunO(_data, sceneContext, renderContext, _output.edit<O>());
                }
            }
        };

        template <class T, class I, class O> class ModelIO : public Concept {
        public:
            typedef T Data;
            typedef I Input;
            typedef O Output;

            Data _data;
            Varying _input;
            Varying _output;

            const Varying getInput() const { return _input; }
            const Varying getOutput() const { return _output; }

            ModelIO(const Varying& input, Data data = Data()) : _data(data), _input(input), _output(Output()) {}
            ModelIO(Data data) : _data(data), _output(Output()) {}

            void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
                if (isEnabled()) {
                    jobRunIO(_data, sceneContext, renderContext, _input.get<I>(), _output.edit<O>());
                }
            }
        };

        Job(ConceptPointer concept) : _concept(concept) {}
        Job(std::string name, ConceptPointer concept) : _concept(concept), _name(name) {}

        bool isEnabled() const { return _concept->isEnabled(); }
        void setEnabled(bool isEnabled) { _concept->setEnabled(isEnabled); }

        const Varying getInput() const { return _concept->getInput(); }
        const Varying getOutput() const { return _concept->getOutput(); }

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

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
            PerformanceTimer perfTimer(_name.c_str());
            PROFILE_RANGE(_name.c_str());

            _concept->run(sceneContext, renderContext);
        }
    protected:
        ConceptPointer _concept;
        std::string _name = "";

    };
    using Jobs = std::vector<Job>;

public:
    Task() = default;
    virtual ~Task() = default;

    // Queue a new job to the task; returns the job's index
    template <class T, class... A> const Varying addJob(std::string name, A&&... args) {
        _jobs.emplace_back(name, std::make_shared<typename T::JobModel>(std::forward<A>(args)...));
        return _jobs.back().getOutput();
    }

    void enableJob(size_t jobIndex, bool enable = true) { _jobs.at(jobIndex).setEnabled(enable); }
    bool getEnableJob(size_t jobIndex) const { return _jobs.at(jobIndex).isEnabled(); }

    virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {}

protected:
    Jobs _jobs;
};
typedef std::shared_ptr<Task> TaskPointer;
typedef std::vector<TaskPointer> Tasks;

}

#endif // hifi_render_Task_h
