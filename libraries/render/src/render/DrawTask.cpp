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

#include <LogHandler.h>

#include "DrawTask.h"
#include "Logging.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>


#include <drawItemBounds_vert.h>
#include <drawItemBounds_frag.h>

using namespace render;

void render::renderItems(const RenderContextPointer& renderContext, const ItemBounds& inItems, int maxDrawnItems) {
    auto& scene = renderContext->_scene;
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

void renderShape(RenderArgs* args, const ShapePlumberPointer& shapeContext, const Item& item, const ShapeKey& globalKey) {
    assert(item.getKey().isShape());
    auto key = item.getShapeKey() | globalKey;
    args->_itemShapeKey = key._flags.to_ulong();
    if (key.isValid() && !key.hasOwnPipeline()) {
        args->_shapePipeline = shapeContext->pickPipeline(args, key);
        if (args->_shapePipeline) {
            args->_shapePipeline->prepareShapeItem(args, key, item);
            item.render(args);
        }
        args->_shapePipeline = nullptr;
    } else if (key.hasOwnPipeline()) {
        item.render(args);
    } else {
        qCDebug(renderlogging) << "Item could not be rendered with invalid key" << key;
        static QString repeatedCouldNotBeRendered = LogHandler::getInstance().addRepeatedMessageRegex(
            "Item could not be rendered with invalid key.*");
    }
    args->_itemShapeKey = 0;
}

void render::renderShapes(const RenderContextPointer& renderContext,
    const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems, const ShapeKey& globalKey) {
    auto& scene = renderContext->_scene;
    RenderArgs* args = renderContext->args;

    int numItemsToDraw = (int)inItems.size();
    if (maxDrawnItems != -1) {
        numItemsToDraw = glm::min(numItemsToDraw, maxDrawnItems);
    }
    for (auto i = 0; i < numItemsToDraw; ++i) {
        auto& item = scene->getItem(inItems[i].id);
        renderShape(args, shapeContext, item, globalKey);
    }
}

void render::renderStateSortShapes(const RenderContextPointer& renderContext,
    const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems, const ShapeKey& globalKey) {
    auto& scene = renderContext->_scene;
    RenderArgs* args = renderContext->args;

    int numItemsToDraw = (int)inItems.size();
    if (maxDrawnItems != -1) {
        numItemsToDraw = glm::min(numItemsToDraw, maxDrawnItems);
    }

    using SortedPipelines = std::vector<render::ShapeKey>;
    using SortedShapes = std::unordered_map<render::ShapeKey, std::vector<Item>, render::ShapeKey::Hash, render::ShapeKey::KeyEqual>;
    SortedPipelines sortedPipelines;
    SortedShapes sortedShapes;
    std::vector< std::tuple<Item,ShapeKey> > ownPipelineBucket;

    for (auto i = 0; i < numItemsToDraw; ++i) {
        auto& item = scene->getItem(inItems[i].id);

        {
            assert(item.getKey().isShape());
            auto key = item.getShapeKey() | globalKey;
            if (key.isValid() && !key.hasOwnPipeline()) {
                auto& bucket = sortedShapes[key];
                if (bucket.empty()) {
                    sortedPipelines.push_back(key);
                }
                bucket.push_back(item);
            } else if (key.hasOwnPipeline()) {
                ownPipelineBucket.push_back( std::make_tuple(item, key) );
            } else {
                static QString repeatedCouldNotBeRendered = LogHandler::getInstance().addRepeatedMessageRegex(
                    "Item could not be rendered with invalid key.*");
                qCDebug(renderlogging) << "Item could not be rendered with invalid key" << key;
            }
        }
    }

    // Then render
    for (auto& pipelineKey : sortedPipelines) {
        auto& bucket = sortedShapes[pipelineKey];
        args->_shapePipeline = shapeContext->pickPipeline(args, pipelineKey);
        if (!args->_shapePipeline) {
            continue;
        }
        args->_itemShapeKey = pipelineKey._flags.to_ulong();
        for (auto& item : bucket) {
            args->_shapePipeline->prepareShapeItem(args, pipelineKey, item);
            item.render(args);
        }
    }
    args->_shapePipeline = nullptr;
    for (auto& itemAndKey : ownPipelineBucket) {
        auto& item = std::get<0>(itemAndKey);
        args->_itemShapeKey = std::get<1>(itemAndKey)._flags.to_ulong();
        item.render(args);
    }
    args->_itemShapeKey = 0;
}

void DrawLight::run(const RenderContextPointer& renderContext, const ItemBounds& inLights) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    // render lights
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        renderItems(renderContext, inLights, _maxDrawn);
        args->_batch = nullptr;
    });

    auto config = std::static_pointer_cast<Config>(renderContext->jobConfig);
    config->setNumDrawn((int)inLights.size());
}

const gpu::PipelinePointer DrawBounds::getPipeline() {
    if (!_boundsPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(drawItemBounds_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawItemBounds_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        _colorLocation = program->getUniforms().findLocation("inColor");

        auto state = std::make_shared<gpu::State>();
        state->setDepthTest(true, false, gpu::LESS_EQUAL);
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        _boundsPipeline = gpu::Pipeline::create(program, state);
    }
    return _boundsPipeline;
}

void DrawBounds::run(const RenderContextPointer& renderContext,
    const Inputs& items) {
    RenderArgs* args = renderContext->args;

    uint32_t numItems = (uint32_t) items.size();
    if (numItems == 0) {
        return;
    }

    static const uint32_t sizeOfItemBound = sizeof(ItemBound);
    if (!_drawBuffer) {
        _drawBuffer = std::make_shared<gpu::Buffer>(sizeOfItemBound);
    }

    _drawBuffer->setData(numItems * sizeOfItemBound, (const gpu::Byte*) items.data());

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;

        // Setup projection
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // Bind program
        batch.setPipeline(getPipeline());

        glm::vec4 color(glm::vec3(0.0f), -(float) numItems);
        batch._glUniform4fv(_colorLocation, 1, (const float*)(&color));
        batch.setResourceBuffer(0, _drawBuffer);

        static const int NUM_VERTICES_PER_CUBE = 24;
        batch.draw(gpu::LINES, NUM_VERTICES_PER_CUBE * numItems, 0);
    });
}

