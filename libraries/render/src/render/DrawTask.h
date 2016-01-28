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

using CullFunctor = std::function<bool(const RenderArgs*, const AABox&)>;

void cullItems(const RenderContextPointer& renderContext, const CullFunctor& cullFunctor, RenderDetails::Item& details,
               const ItemBounds& inItems, ItemBounds& outItems);
void depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemBounds& inItems, ItemBounds& outItems);
void renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems);
void renderShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems = -1);

class FetchItemsConfig : public Job::Config {
    Q_OBJECT
    Q_PROPERTY(int numItems READ getNumItems)
public:
    int getNumItems() { return numItems; }

    int numItems{ 0 };
};

class FetchItems {
public:
    using Config = FetchItemsConfig;
    using JobModel = Job::ModelO<FetchItems, ItemBounds, Config>;

    FetchItems() {}
    FetchItems(const ItemFilter& filter) : _filter(filter) {}

    ItemFilter _filter{ ItemFilter::Builder::opaqueShape().withoutLayered() };

    void configure(const Config& config) {}
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemBounds& outItems);
};

template<RenderDetails::Type T>
class CullItems {
public:
    using JobModel = Job::ModelIO<CullItems<T>, ItemBounds, ItemBounds>;
  
    CullItems(CullFunctor cullFunctor) : _cullFunctor{ cullFunctor } {}

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
        const auto& args = renderContext->args;
        auto& details = args->_details.edit(T);
        outItems.clear();
        outItems.reserve(inItems.size());
        render::cullItems(renderContext, _cullFunctor, details, inItems, outItems);
    }

protected:
    CullFunctor _cullFunctor;
};

class DepthSortItems {
public:
    using JobModel = Job::ModelIO<DepthSortItems, ItemBounds, ItemBounds>;
 
    bool _frontToBack;
    DepthSortItems(bool frontToBack = true) : _frontToBack(frontToBack) {}

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems);
};

class DrawLight {
public:
    using JobModel = Job::Model<DrawLight>;

    DrawLight(CullFunctor cullFunctor) : _cullFunctor{ cullFunctor } {}
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext);
protected:
    CullFunctor _cullFunctor;
};

class PipelineSortShapes {
public:
    using JobModel = Job::ModelIO<PipelineSortShapes, ItemBounds, ShapesIDsBounds>;
    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ShapesIDsBounds& outShapes);
};

class DepthSortShapes {
public:
    using JobModel = Job::ModelIO<DepthSortShapes, ShapesIDsBounds, ShapesIDsBounds>;
 
    bool _frontToBack;
    DepthSortShapes(bool frontToBack = true) : _frontToBack(frontToBack) {}

    void run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapesIDsBounds& inShapes, ShapesIDsBounds& outShapes);
};

}

#endif // hifi_render_DrawTask_h
