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

#ifndef hifi_render_DrawTask_h
#define hifi_render_DrawTask_h

#include "Engine.h"
#include "gpu/Batch.h"


namespace render {

void cullItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outITems);
void depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemIDsBounds& inItems, ItemIDsBounds& outITems);
void renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, int maxDrawnItems = -1);


class FetchItems {
public:
    typedef std::function<void (const RenderContextPointer& context, int count)> ProbeNumItems;
    FetchItems(const ProbeNumItems& probe): _probeNumItems(probe) {}
    FetchItems(const ItemFilter& filter, const ProbeNumItems& probe): _filter(filter), _probeNumItems(probe) {}

    ItemFilter _filter = ItemFilter::Builder::opaqueShape().withoutLayered();
    ProbeNumItems _probeNumItems;

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemIDsBounds& outItems);

    using JobModel = Task::Job::ModelO<FetchItems, ItemIDsBounds>;
};

class CullItems {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);
    using JobModel = Task::Job::ModelIO<CullItems, ItemIDsBounds, ItemIDsBounds>;
};

class CullItemsOpaque {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);
    using JobModel = Task::Job::ModelIO<CullItemsOpaque, ItemIDsBounds, ItemIDsBounds>;
};

class CullItemsTransparent {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);
    using JobModel = Task::Job::ModelIO<CullItemsTransparent, ItemIDsBounds, ItemIDsBounds>;
};

class DepthSortItems {
public:
    bool _frontToBack = true;

    DepthSortItems(bool frontToBack = true) : _frontToBack(frontToBack) {}

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outITems);

    using JobModel = Task::Job::ModelIO<DepthSortItems, ItemIDsBounds, ItemIDsBounds>;
};

class DrawLight {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);

    using JobModel = Task::Job::Model<DrawLight>;
};

class DrawBackground {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);


    typedef Job::Model<DrawBackground> JobModel;
};


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
void materialSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);

}

#endif // hifi_render_DrawTask_h
