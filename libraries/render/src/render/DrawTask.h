//
//  DrawTask.h
//  render/src/render
//
//  Created by Sam Gateau on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_Task_h
#define hifi_render_Task_h

#include "Engine.h"

namespace render {





template <class T> void jobRun(const T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) { }
template <class T, class I> void jobRunI(const T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input) { }
template <class T, class O> void jobRunO(const T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, O& output) {
    jobModel.run(sceneContext, renderContext, output);
}
template <class T, class I, class O> void jobRunIO(const T& jobModel, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const I& input, O& output) { }

class Job {
public:

    class Varying {
    public:

        Varying() {}
        template <class T> 
        Varying(T data) : _concept(new Model<T>(data)) {}

    protected:
        friend class Job;

        std::vector<std::weak_ptr<Job>> _consumerJobs;

        void addJobConsumer(const std::shared_ptr<Job>& job) {
            _consumerJobs.push_back(job);
        }
 
        class Concept {
        public:
            virtual ~Concept() = default;
        };
        template <class T> class Model : public Concept {
        public:
            typedef T Data;
            Data _data;
            Model(Data data): _data(data) {}
        };
    };
    typedef std::shared_ptr<Varying> VaryingPointer;

    template <class T> 
    Job(T data) : _concept(new Model<T>(data)) {}


    template <class T, class O> 
    static Job* createO(T data) { return new Job(new ModelO<T, O>(data)); }

    Job(const Job& other) : _concept(other._concept) {}
    ~Job();

    const VaryingPointer& getInput() const { return _concept->getInput(); }
    const VaryingPointer& getOutput() const { return _concept->getOutput(); }

    virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
        if (_concept) {
            _concept->run(sceneContext, renderContext);
        }
    }

protected:

    class Concept {
    public:
        virtual ~Concept() = default;
        virtual const VaryingPointer& getInput() const { return VaryingPointer(); }
        virtual const VaryingPointer& getOutput() const { return VaryingPointer(); }
        virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) = 0;
    };

    Job(Concept* concept) : _concept(concept) {}

    template <class T> class Model : public Concept {
    public:
        typedef T Data;

        Data _data;

        Model(Data data): _data(data) {}

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) { jobRun(_data, sceneContext, renderContext); }
    };
public:
    template <class T, class I> class ModelI : public Concept {
    public:
        typedef T Data;
        typedef I Input;

        Data _data;
        VaryingPointer _intput;

        ModelI(Data data): _data(data) {}

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) { jobRunI(_data, sceneContext, renderContext, _input); }
    };

    template <class T, class O> class ModelO : public Concept {
    public:
        typedef T Data;
        typedef O Output;

        Data _data;
        VaryingPointer _output;

        ModelO(Data data): _data(data), _output(new Varying::Model<O>(Output())) {}

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) { jobRunO(_data, sceneContext, renderContext, _output); }
    };

    template <class T, class I, class O> class ModelIO : public Concept {
    public:
        typedef T Data;
        typedef I Input;
        typedef O Output;

        Data _data;
        VaryingPointer _intput;
        VaryingPointer _output;

        ModelIO(Data data, Output output): _data(data), _output(new Varying::Model<O>(output)) {}

        void setInput(const VaryingPointer& input) { _input = input; }

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) { jobRunIO(_data, sceneContext, renderContext, _input, _output); }
    };

    std::shared_ptr<Concept> _concept;
};




typedef std::vector<Job> Jobs;

void cullItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outITems);
void depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemIDsBounds& inItems, ItemIDsBounds& outITems);
void renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, int maxDrawnItems = -1);

class CulledItems {
public:
    ItemIDsBounds _items;
};

class FetchCullItems {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, CulledItems& outItems);

    typedef Job::ModelO<FetchCullItems, CulledItems> JobModel;
};



void materialSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);


class DrawOpaque {
public:
};
template <> void jobRun(const DrawOpaque& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);


class DrawTransparent {
public:
};
template <> void jobRun(const DrawTransparent& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);

class DrawLight {
public:
};
template <> void jobRun(const DrawLight& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);

class DrawBackground {
public:
};
template <> void jobRun(const DrawBackground& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);


class DrawPostLayered {
public:
};
template <> void jobRun(const DrawPostLayered& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);



class ResetGLState {
public:
};
template <> void jobRun(const ResetGLState& job, const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);


class DrawSceneTask : public Task {
public:

    DrawSceneTask();
    ~DrawSceneTask();

    Jobs _jobs;

    virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);

};


// A map of ItemIDs allowing to create bucket lists of SHAPE type items which are filtered by their
// Material 
class ItemMaterialBucketMap : public std::map<model::MaterialFilter, ItemIDs, model::MaterialFilter::Less> {
public:

    ItemMaterialBucketMap() {}

    void insert(const ItemID& id, const model::MaterialKey& key);

    // standard builders allocating the main buckets
    void allocateStandardMaterialBuckets();
};

}

#endif // hifi_render_Task_h
