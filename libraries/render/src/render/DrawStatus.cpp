//
//  DrawStatus.cpp
//  render/src/render
//
//  Created by Niraj Venkat on 6/29/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DrawStatus.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <ViewFrustum.h>

#include <gpu/Context.h>

#include <shaders/Shaders.h>

#include "Args.h"

using namespace render;

void DrawStatusConfig::dirtyHelper() {
    enabled = showNetwork || showDisplay;
    emit dirty();
}

const gpu::PipelinePointer DrawStatus::getDrawItemBoundsPipeline() {
    if (!_drawItemBoundsPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render::program::drawItemBounds);

        auto state = std::make_shared<gpu::State>();

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Good to go add the brand new pipeline
        _drawItemBoundsPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawItemBoundsPipeline;
}

const gpu::PipelinePointer DrawStatus::getDrawItemStatusPipeline() {
    if (!_drawItemStatusPipeline) {
        gpu::ShaderPointer program = gpu::Shader::createProgram(shader::render::program::drawItemStatus);

        auto state = std::make_shared<gpu::State>();

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Good to go add the brand new pipeline
        _drawItemStatusPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawItemStatusPipeline;
}

void DrawStatus::setStatusIconMap(const gpu::TexturePointer& map) {
    _statusIconMap = map;
}

const gpu::TexturePointer DrawStatus::getStatusIconMap() const {
    return _statusIconMap;
}

void DrawStatus::configure(const Config& config) {
    _showDisplay = config.showDisplay;
    _showNetwork = config.showNetwork;
}

void DrawStatus::run(const RenderContextPointer& renderContext, const Input& input) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& scene = renderContext->_scene;
    const int NUM_STATUS_VEC4_PER_ITEM = 2;
    const int VEC4_LENGTH = 4;

    const auto& inItems = input.get0();
    const auto jitter = input.get1();

    // First thing, we collect the bound and the status for all the items we want to render
    int nbItems = 0;
    render::ItemBounds itemBounds;
    std::vector<std::pair<glm::ivec4, glm::ivec4>> itemStatus;
    {
        for (size_t i = 0; i < inItems.size(); ++i) {
            const auto& item = inItems[i];
            if (!item.bound.isInvalid()) {
                if (!item.bound.isNull()) {
                    itemBounds.emplace_back(render::ItemBound(item.id, item.bound));
                } else {
                    itemBounds.emplace_back(item.id, AABox(item.bound.getCorner(), 0.1f));
                }

                auto& itemScene = scene->getItem(item.id);

                auto itemStatusPointer = itemScene.getStatus();
                if (itemStatusPointer) {
                    itemStatus.push_back(std::pair<glm::ivec4, glm::ivec4>());
                    // Query the current status values, this is where the statusGetter lambda get called
                    auto&& currentStatusValues = itemStatusPointer->getCurrentValues();
                    int valueNum = 0;
                    for (int vec4Num = 0; vec4Num < NUM_STATUS_VEC4_PER_ITEM; vec4Num++) {
                        auto& value = (vec4Num ? itemStatus[nbItems].first : itemStatus[nbItems].second);
                        value = glm::ivec4(Item::Status::Value::INVALID.getPackedData());
                        for (int component = 0; component < VEC4_LENGTH; component++) {
                            valueNum = vec4Num * VEC4_LENGTH + component;
                            if (valueNum < (int)currentStatusValues.size()) {
                                value[component] = currentStatusValues[valueNum].getPackedData();
                            }
                        }
                    }
                } else {
                    auto invalid = glm::ivec4(Item::Status::Value::INVALID.getPackedData());
                    itemStatus.emplace_back(invalid, invalid);
                }
                nbItems++;
            }
        }
    }

    if (nbItems == 0) {
        return;
    }

    if (!_boundsBuffer) {
        _boundsBuffer = std::make_shared<gpu::Buffer>(sizeof(render::ItemBound));
    }

    // Alright, something to render let's do it
    gpu::doInBatch("DrawStatus::run", args->_context, [&](gpu::Batch& batch) {
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setViewportTransform(args->_viewport);

        batch.setProjectionTransform(projMat);
        batch.setProjectionJitter(jitter.x, jitter.y);
        batch.setViewTransform(viewMat, true);
        batch.setModelTransform(Transform());

        // bind the one gpu::Pipeline we need
        batch.setPipeline(getDrawItemBoundsPipeline());

        _boundsBuffer->setData(itemBounds.size() * sizeof(render::ItemBound), (const gpu::Byte*) itemBounds.data());

        if (_showDisplay) {
            batch.setResourceBuffer(0, _boundsBuffer);
            batch.draw(gpu::LINES, (gpu::uint32) itemBounds.size() * 24, 0);
        }
        batch.setResourceBuffer(0, 0);

        batch.setResourceTexture(0, gpu::TextureView(getStatusIconMap(), 0));

        batch.setPipeline(getDrawItemStatusPipeline());

        if (_showNetwork) {
            if (!_instanceBuffer) {
                _instanceBuffer = std::make_shared<gpu::Buffer>();
            }

            struct InstanceData {
                vec4 boundPos;
                vec4 boundDim;
                ivec4 status0;
                ivec4 status1;
            };

            if (!_vertexFormat) {
                _vertexFormat = std::make_shared<gpu::Stream::Format>();
                _vertexFormat->setAttribute(0, 0, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW), offsetof(InstanceData, boundPos), gpu::Stream::PER_INSTANCE);
                _vertexFormat->setAttribute(1, 0, gpu::Element(gpu::VEC4, gpu::FLOAT, gpu::XYZW), offsetof(InstanceData, boundDim), gpu::Stream::PER_INSTANCE);
                _vertexFormat->setAttribute(2, 0, gpu::Element(gpu::VEC4, gpu::INT32, gpu::XYZW), offsetof(InstanceData, status0), gpu::Stream::PER_INSTANCE);
                _vertexFormat->setAttribute(3, 0, gpu::Element(gpu::VEC4, gpu::INT32, gpu::XYZW), offsetof(InstanceData, status1), gpu::Stream::PER_INSTANCE);
            }

            batch.setInputFormat(_vertexFormat);
            std::vector<InstanceData> instanceData;
            instanceData.resize(itemBounds.size());
            for (size_t i = 0; i < itemBounds.size(); i++) {
                InstanceData& item = instanceData[i];
                const auto& bound = itemBounds[i].bound;
                const auto& status = itemStatus[i];
                item.boundPos = vec4(bound.getCorner(), 1.0f);
                item.boundDim = vec4(bound.getScale(), 1.0f);
                item.status0 = status.first;
                item.status1 = status.second;
            }

            auto instanceBufferSize = sizeof(InstanceData) * instanceData.size();
            if (_instanceBuffer->getSize() < instanceBufferSize) {
                _instanceBuffer->resize(instanceBufferSize);
            }
            _instanceBuffer->setSubData(0, instanceData);
            batch.setInputBuffer(0, _instanceBuffer, 0, sizeof(InstanceData));
            batch.drawInstanced((uint32_t)instanceData.size(), gpu::TRIANGLES, 24 * NUM_STATUS_VEC4_PER_ITEM);
        }
        batch.setResourceTexture(0, 0);
    });
}
