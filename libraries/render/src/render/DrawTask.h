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

class Job {
public:

    template <class T> 
    Job(T data) : _concept(new Model<T>(data)) {}
    Job(const Job& other) : _concept(other._concept) {}
    ~Job();

    virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
        if (_concept) {
            _concept->run(sceneContext, renderContext);
        }
    }

protected:
    class Concept {
    public:
        virtual ~Concept() = default;
        virtual void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) = 0;
    };

    template <class T> class Model : public Concept {
    public:
        typedef T Data;
    
        Data _data;
        Model(Data data): _data(data) {}

        void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) { jobRun(_data, sceneContext, renderContext); }
    };

    std::shared_ptr<Concept> _concept;
};




typedef std::vector<Job> Jobs;

void cullItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outITems);
void depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemIDsBounds& inItems, ItemIDsBounds& outITems);
void renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, int maxDrawnItems = -1);



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
