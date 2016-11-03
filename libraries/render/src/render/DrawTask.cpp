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

void render::renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems, int maxDrawnItems) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;

    int numItemsToDraw = (int)inItems.size();
    if (maxDrawnItems != -1) {
        numItemsToDraw = glm::min(numItemsToDraw, maxDrawnItems);
    }
    for (auto i = 0; i < numItemsToDraw; ++i) {
        auto& item = scene->getItem(inItems[i].id);
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
        args->_pipeline = nullptr;
    } else if (key.hasOwnPipeline()) {
        item.render(args);
    } else {
        qDebug() << "Item could not be rendered with invalid key" << key;
    }
}

void render::renderShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext,
                          const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;
    
    int numItemsToDraw = (int)inItems.size();
    if (maxDrawnItems != -1) {
        numItemsToDraw = glm::min(numItemsToDraw, maxDrawnItems);
    }
    for (auto i = 0; i < numItemsToDraw; ++i) {
        auto& item = scene->getItem(inItems[i].id);
        renderShape(args, shapeContext, item);
    }
}

void render::renderStateSortShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext,
    const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;

    int numItemsToDraw = (int)inItems.size();
    if (maxDrawnItems != -1) {
        numItemsToDraw = glm::min(numItemsToDraw, maxDrawnItems);
    }

    using SortedPipelines = std::vector<render::ShapeKey>;
    using SortedShapes = std::unordered_map<render::ShapeKey, std::vector<Item>, render::ShapeKey::Hash, render::ShapeKey::KeyEqual>;
    SortedPipelines sortedPipelines;
    SortedShapes sortedShapes;
    std::vector<Item> ownPipelineBucket;

    for (auto i = 0; i < numItemsToDraw; ++i) {
        auto item = scene->getItem(inItems[i].id);

        {
            assert(item.getKey().isShape());
            const auto key = item.getShapeKey();
            if (key.isValid() && !key.hasOwnPipeline()) {
                auto& bucket = sortedShapes[key];
                if (bucket.empty()) {
                    sortedPipelines.push_back(key);
                }
                bucket.push_back(item);
            } else if (key.hasOwnPipeline()) {
                ownPipelineBucket.push_back(item);
            } else {
                qDebug() << "Item could not be rendered with invalid key" << key;
            }
        }
    }

    // Then render
    for (auto& pipelineKey : sortedPipelines) {
        auto& bucket = sortedShapes[pipelineKey];
        args->_pipeline = shapeContext->pickPipeline(args, pipelineKey);
        if (!args->_pipeline) {
            continue;
        }
        for (auto& item : bucket) {
            item.render(args);
        }
    }
    args->_pipeline = nullptr;
    for (auto& item : ownPipelineBucket) {
        item.render(args);
    }
}

void DrawLight::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inLights) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    // render lights
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        renderItems(sceneContext, renderContext, inLights, _maxDrawn);
        args->_batch = nullptr;
    });

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setNumDrawn((int)inLights.size());
}
