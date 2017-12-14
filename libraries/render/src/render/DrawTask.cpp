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
#include <gpu/StandardShaderLib.h>

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

DrawQuadVolume::DrawQuadVolume(const glm::vec3& color) :
    _color{ color } {
    _meshVertices = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(glm::vec3) * 8, nullptr), gpu::Element::VEC3F_XYZ);
    _meshStream.addBuffer(_meshVertices._buffer, _meshVertices._offset, _meshVertices._stride);
}

void DrawQuadVolume::configure(const Config& configuration) {
    _isUpdateEnabled = !configuration.isFrozen;
}

void DrawQuadVolume::run(const render::RenderContextPointer& renderContext, const glm::vec3 vertices[8],
                         const gpu::BufferView& indices, int indexCount) {
    assert(renderContext->args);
    assert(renderContext->args->_context);
    if (_isUpdateEnabled) {
        auto& streamVertices = _meshVertices.edit<std::array<glm::vec3, 8U> >();
        std::copy(vertices, vertices + 8, streamVertices.begin());
    }

    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        args->_batch = &batch;
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setPipeline(getPipeline());
        batch.setIndexBuffer(indices);

        batch._glUniform4f(0, _color.x, _color.y, _color.z, 1.0f);
        batch.setInputStream(0, _meshStream);
        batch.drawIndexed(gpu::LINES, indexCount, 0U);

        args->_batch = nullptr;
    });
}

gpu::PipelinePointer DrawQuadVolume::getPipeline() {
    static gpu::PipelinePointer pipeline;

    if (!pipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformVertexPositionVS();
        auto ps = gpu::StandardShaderLib::getDrawColorPS();
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("color", 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        state->setDepthTest(gpu::State::DepthTest(true, false));
        pipeline = gpu::Pipeline::create(program, state);
    }
    return pipeline;
}

gpu::BufferView DrawAABox::_cubeMeshIndices;

DrawAABox::DrawAABox(const glm::vec3& color) :
    DrawQuadVolume{ color } {
}

void DrawAABox::run(const render::RenderContextPointer& renderContext, const Inputs& box) {
    if (!box.isNull()) {
        static const uint8_t indexData[] = {
            0, 1,
            1, 2,
            2, 3,
            3, 0,
            4, 5,
            5, 6,
            6, 7,
            7, 4,
            0, 4,
            1, 5,
            3, 7,
            2, 6
        };

        if (!_cubeMeshIndices._buffer) {
            auto indices = std::make_shared<gpu::Buffer>(sizeof(indexData), indexData);
            _cubeMeshIndices = gpu::BufferView(indices, gpu::Element(gpu::SCALAR, gpu::UINT8, gpu::INDEX));
        }

        glm::vec3 vertices[8];

        getVertices(box, vertices);

        DrawQuadVolume::run(renderContext, vertices, _cubeMeshIndices, sizeof(indexData) / sizeof(indexData[0]));
    }
}

void DrawAABox::getVertices(const AABox& box, glm::vec3 vertices[8]) {
    vertices[0] = box.getVertex(TOP_LEFT_NEAR);
    vertices[1] = box.getVertex(TOP_RIGHT_NEAR);
    vertices[2] = box.getVertex(BOTTOM_RIGHT_NEAR);
    vertices[3] = box.getVertex(BOTTOM_LEFT_NEAR);
    vertices[4] = box.getVertex(TOP_LEFT_FAR);
    vertices[5] = box.getVertex(TOP_RIGHT_FAR);
    vertices[6] = box.getVertex(BOTTOM_RIGHT_FAR);
    vertices[7] = box.getVertex(BOTTOM_LEFT_FAR);
}

gpu::BufferView DrawFrustum::_frustumMeshIndices;

DrawFrustum::DrawFrustum(const glm::vec3& color) :
    DrawQuadVolume{ color } {
}

void DrawFrustum::run(const render::RenderContextPointer& renderContext, const Input& input) {
    if (input) {
        const auto& frustum = *input;

        static const uint8_t indexData[] = { 
            0, 1, 
            1, 2, 
            2, 3, 
            3, 0, 
            0, 2, 
            3, 1,
            4, 5,
            5, 6,
            6, 7,
            7, 4,
            4, 6,
            7, 5,
            0, 4,
            1, 5,
            3, 7,
            2, 6
        };

        if (!_frustumMeshIndices._buffer) {
            auto indices = std::make_shared<gpu::Buffer>(sizeof(indexData), indexData);
            _frustumMeshIndices = gpu::BufferView(indices, gpu::Element(gpu::SCALAR, gpu::UINT8, gpu::INDEX));
        }

        glm::vec3 vertices[8];

        getVertices(frustum, vertices);

        DrawQuadVolume::run(renderContext, vertices, _frustumMeshIndices, sizeof(indexData) / sizeof(indexData[0]));
    }
}

void DrawFrustum::getVertices(const ViewFrustum& frustum, glm::vec3 vertices[8]) {
    vertices[0] = frustum.getNearTopLeft();
    vertices[1] = frustum.getNearTopRight();
    vertices[2] = frustum.getNearBottomRight();
    vertices[3] = frustum.getNearBottomLeft();
    vertices[4] = frustum.getFarTopLeft();
    vertices[5] = frustum.getFarTopRight();
    vertices[6] = frustum.getFarBottomRight();
    vertices[7] = frustum.getFarBottomLeft();
}
