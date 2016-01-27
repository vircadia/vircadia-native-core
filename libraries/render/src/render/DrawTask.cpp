//
//  DrawTask.cpp
//  render/src/render
//
//  Created by Sam Gateau on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DrawTask.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>

using namespace render;

void render::cullItems(const RenderContextPointer& renderContext, const CullFunctor& cullFunctor, RenderDetails::Item& details,
                       const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    RenderArgs* args = renderContext->getArgs();
    ViewFrustum* frustum = args->_viewFrustum;

    details._considered += inItems.size();
    
    // Culling / LOD
    for (auto item : inItems) {
        if (item.bound.isNull()) {
            outItems.emplace_back(item); // One more Item to render
            continue;
        }

        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        bool outOfView;
        {
            PerformanceTimer perfTimer("boxInFrustum");
            outOfView = frustum->boxInFrustum(item.bound) == ViewFrustum::OUTSIDE;
        }
        if (!outOfView) {
            bool bigEnoughToRender;
            {
                PerformanceTimer perfTimer("shouldRender");
                bigEnoughToRender = cullFunctor(args, item.bound);
            }
            if (bigEnoughToRender) {
                outItems.emplace_back(item); // One more Item to render
            } else {
                details._tooSmall++;
            }
        } else {
            details._outOfView++;
        }
    }
    details._rendered += outItems.size();
}

struct ItemBoundSort {
    float _centerDepth = 0.0f;
    float _nearDepth = 0.0f;
    float _farDepth = 0.0f;
    ItemID _id = 0;
    AABox _bounds;

    ItemBoundSort() {}
    ItemBoundSort(float centerDepth, float nearDepth, float farDepth, ItemID id, const AABox& bounds) : _centerDepth(centerDepth), _nearDepth(nearDepth), _farDepth(farDepth), _id(id), _bounds(bounds) {}
};

struct FrontToBackSort {
    bool operator() (const ItemBoundSort& left, const ItemBoundSort& right) {
        return (left._centerDepth < right._centerDepth);
    }
};

struct BackToFrontSort {
    bool operator() (const ItemBoundSort& left, const ItemBoundSort& right) {
        return (left._centerDepth > right._centerDepth);
    }
};

void render::depthSortItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, bool frontToBack, const ItemBounds& inItems, ItemBounds& outItems) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);
    
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->getArgs();
    

    // Allocate and simply copy
    outItems.clear();
    outItems.reserve(inItems.size());


    // Make a local dataset of the center distance and closest point distance
    std::vector<ItemBoundSort> itemBoundSorts;
    itemBoundSorts.reserve(outItems.size());

    for (auto itemDetails : inItems) {
        auto item = scene->getItem(itemDetails.id);
        auto bound = itemDetails.bound; // item.getBound();
        float distance = args->_viewFrustum->distanceToCamera(bound.calcCenter());

        itemBoundSorts.emplace_back(ItemBoundSort(distance, distance, distance, itemDetails.id, bound));
    }

    // sort against Z
    if (frontToBack) {
        FrontToBackSort frontToBackSort;
        std::sort(itemBoundSorts.begin(), itemBoundSorts.end(), frontToBackSort);
    } else {
        BackToFrontSort  backToFrontSort;
        std::sort(itemBoundSorts.begin(), itemBoundSorts.end(), backToFrontSort);
    }

    // FInally once sorted result to a list of itemID
    for (auto& item : itemBoundSorts) {
       outItems.emplace_back(ItemBound(item._id, item._bounds));
    }
}

void render::renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->getArgs();

    for (const auto& itemDetails : inItems) {
        auto& item = scene->getItem(itemDetails.id);
        item.render(args);
    }
}

void renderShape(RenderArgs* args, const ShapePlumberPointer& shapeContext, const Item& item) {
    assert(item.getKey().isShape());
    const auto& key = item.getShapeKey();
    if (key.isValid() && !key.hasOwnPipeline()) {
        args->_pipeline = shapeContext->pickPipeline(args, key);
        if (args->_pipeline) {
            item.render(args);
        }
    } else if (key.hasOwnPipeline()) {
        item.render(args);
    } else {
        qDebug() << "Item could not be rendered: invalid key ?" << key;
    }
}

void render::renderShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext,
                          const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->getArgs();
    
    auto numItemsToDraw = glm::max((int)inItems.size(), maxDrawnItems);
    for (auto i = 0; i < numItemsToDraw; ++i) {
        auto& item = scene->getItem(inItems[i].id);
        renderShape(args, shapeContext, item);
    }
}

void FetchItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, ItemBounds& outItems) {
    auto& scene = sceneContext->_scene;

    outItems.clear();

    const auto& bucket = scene->getMasterBucket();
    const auto& items = bucket.find(_filter);
    if (items != bucket.end()) {
        outItems.reserve(items->second.size());
        for (auto& id : items->second) {
            auto& item = scene->getItem(id);
            outItems.emplace_back(ItemBound(id, item.getBound()));
        }
    }

    if (_probeNumItems) {
        _probeNumItems(renderContext, (int)outItems.size());
    }
}

void DepthSortItems::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ItemBounds& outItems) {
    depthSortItems(sceneContext, renderContext, _frontToBack, inItems, outItems);
}

void DrawLight::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);

    // render lights
    auto& scene = sceneContext->_scene;
    auto& items = scene->getMasterBucket().at(ItemFilter::Builder::light());

    ItemBounds inItems;
    inItems.reserve(items.size());
    for (auto id : items) {
        auto item = scene->getItem(id);
        inItems.emplace_back(ItemBound(id, item.getBound()));
    }

    RenderArgs* args = renderContext->getArgs();

    auto& details = args->_details.edit(RenderDetails::OTHER_ITEM);
    ItemBounds culledItems;
    culledItems.reserve(inItems.size());
    cullItems(renderContext, _cullFunctor, details, inItems, culledItems);

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        renderItems(sceneContext, renderContext, culledItems);
        args->_batch = nullptr;
    });
}

void PipelineSortShapes::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, ShapesIDsBounds& outShapes) {
    auto& scene = sceneContext->_scene;
    outShapes.clear();

    for (const auto& item : inItems) {
        auto key = scene->getItem(item.id).getShapeKey();
        auto outItems = outShapes.find(key);
        if (outItems == outShapes.end()) {
            outItems = outShapes.insert(std::make_pair(key, ItemBounds{})).first;
            outItems->second.reserve(inItems.size());
        }

        outItems->second.push_back(item);
    }

    for (auto& items : outShapes) {
        items.second.shrink_to_fit();
    }
}

void DepthSortShapes::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ShapesIDsBounds& inShapes, ShapesIDsBounds& outShapes) {
    outShapes.clear();
    outShapes.reserve(inShapes.size());

    for (auto& pipeline : inShapes) {
        auto& inItems = pipeline.second;
        auto outItems = outShapes.find(pipeline.first);
        if (outItems == outShapes.end()) {
            outItems = outShapes.insert(std::make_pair(pipeline.first, ItemBounds{})).first;
        }

        depthSortItems(sceneContext, renderContext, _frontToBack, inItems, outItems->second);
    }
}