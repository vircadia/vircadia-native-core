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
#include "ShapePipeline.h"
#include "gpu/Batch.h"


namespace render {

void cullItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);
void depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);
void renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems);
void renderShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapePlumberPointer& shapeContext, const ItemIDsBounds& inItems, int maxDrawnItems = -1);

class FetchItems {
public:
    typedef std::function<void (const RenderContextPointer& context, int count)> ProbeNumItems;
    FetchItems() {}
    FetchItems(const ProbeNumItems& probe): _probeNumItems(probe) {}
    FetchItems(const ItemFilter& filter, const ProbeNumItems& probe): _filter(filter), _probeNumItems(probe) {}

    ItemFilter _filter = ItemFilter::Builder::opaqueShape().withoutLayered();
    ProbeNumItems _probeNumItems;

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemIDsBounds& outItems);
    using JobModel = Task::Job::ModelO<FetchItems, ItemIDsBounds>;
};

template<RenderDetails::Type T = RenderDetails::Type::OTHER_ITEM>
class CullItems {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems) {
        outItems.clear();
        outItems.reserve(inItems.size());
        renderContext->getArgs()->_details.pointTo(T);
        render::cullItems(sceneContext, renderContext, inItems, outItems);
    }

    using JobModel = Task::Job::ModelIO<CullItems<T>, ItemIDsBounds, ItemIDsBounds>;
};

class DepthSortItems {
public:
    bool _frontToBack;
    DepthSortItems(bool frontToBack = true) : _frontToBack(frontToBack) {}

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ItemIDsBounds& outItems);
    using JobModel = Task::Job::ModelIO<DepthSortItems, ItemIDsBounds, ItemIDsBounds>;
};

class DrawLight {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);
    using JobModel = Task::Job::Model<DrawLight>;
};

class PipelineSortShapes {
public:
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems, ShapesIDsBounds& outShapes);
    using JobModel = Task::Job::ModelIO<PipelineSortShapes, ItemIDsBounds, ShapesIDsBounds>;
};

class DepthSortShapes {
public:
    bool _frontToBack;
    DepthSortShapes(bool frontToBack = true) : _frontToBack(frontToBack) {}

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapesIDsBounds& inShapes, ShapesIDsBounds& outShapes);
    using JobModel = Task::Job::ModelIO<DepthSortShapes, ShapesIDsBounds, ShapesIDsBounds>;
};

}

#endif // hifi_render_DrawTask_h
