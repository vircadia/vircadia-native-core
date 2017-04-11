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
#include "Logging.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <ViewFrustum.h>
#include <gpu/Context.h>


#include <drawItemBounds_vert.h>
#include <drawItemBounds_frag.h>

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

void renderShape(RenderArgs* args, const ShapePlumberPointer& shapeContext, const Item& item, const ShapeKey& globalKey) {
    assert(item.getKey().isShape());
    auto key = item.getShapeKey() | globalKey;
    if (key.isValid() && !key.hasOwnPipeline()) {
        args->_pipeline = shapeContext->pickPipeline(args, key);
        if (args->_pipeline) {
            item.render(args);
        }
        args->_pipeline = nullptr;
    } else if (key.hasOwnPipeline()) {
        item.render(args);
    } else {
        qCDebug(renderlogging) << "Item could not be rendered with invalid key" << key;
    }
}

void render::renderShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext,
    const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems, const ShapeKey& globalKey) {
    auto& scene = sceneContext->_scene;
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

void render::renderStateSortShapes(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext,
    const ShapePlumberPointer& shapeContext, const ItemBounds& inItems, int maxDrawnItems, const ShapeKey& globalKey) {
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
            auto key = item.getShapeKey() | globalKey;
            if (key.isValid() && !key.hasOwnPipeline()) {
                auto& bucket = sortedShapes[key];
                if (bucket.empty()) {
                    sortedPipelines.push_back(key);
                }
                bucket.push_back(item);
            } else if (key.hasOwnPipeline()) {
                ownPipelineBucket.push_back(item);
            } else {
                qCDebug(renderlogging) << "Item could not be rendered with invalid key" << key;
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

const gpu::PipelinePointer DrawBounds::getPipeline() {
    if (!_boundsPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(drawItemBounds_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawItemBounds_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        _cornerLocation = program->getUniforms().findLocation("inBoundPos");
        _scaleLocation = program->getUniforms().findLocation("inBoundDim");
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

void DrawBounds::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext,
    const Inputs& items) {
    RenderArgs* args = renderContext->args;

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
        assert(_cornerLocation >= 0);
        assert(_scaleLocation >= 0);

        // Render bounds
        float numItems = (float) items.size();
        float itemNum = 0.0f;
        for (const auto& item : items) {
            glm::vec4 color(glm::vec3(itemNum / numItems), 1.0f);
            batch._glUniform3fv(_cornerLocation, 1, (const float*)(&item.bound.getCorner()));
            batch._glUniform3fv(_scaleLocation, 1, (const float*)(&item.bound.getScale()));
            batch._glUniform4fv(_colorLocation, 1, (const float*)(&color));

            static const int NUM_VERTICES_PER_CUBE = 24;
            batch.draw(gpu::LINES, NUM_VERTICES_PER_CUBE, 0);
            itemNum += 1.0f;
        }
    });
}

