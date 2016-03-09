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

void render::renderItems(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inItems) {
    auto& scene = sceneContext->_scene;
    RenderArgs* args = renderContext->args;

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
        args->_pipeline = nullptr;
    } else if (key.hasOwnPipeline()) {
        item.render(args);
    } else {
        qDebug() << "Item could not be rendered: invalid key ?" << key;
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

void DrawLight::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemBounds& inLights) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;

    // render lights
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        renderItems(sceneContext, renderContext, inLights);
        args->_batch = nullptr;
    });
}
