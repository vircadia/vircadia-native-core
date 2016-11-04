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
#include <RenderArgs.h>

#include <gpu/Context.h>

#include "drawItemBounds_vert.h"
#include "drawItemBounds_frag.h"
#include "drawItemStatus_vert.h"
#include "drawItemStatus_frag.h"

using namespace render;

void DrawStatusConfig::dirtyHelper() {
    enabled = showNetwork || showDisplay;
    emit dirty();
}

const gpu::PipelinePointer DrawStatus::getDrawItemBoundsPipeline() {
    if (!_drawItemBoundsPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(drawItemBounds_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawItemBounds_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        _drawItemBoundPosLoc = program->getUniforms().findLocation("inBoundPos");
        _drawItemBoundDimLoc = program->getUniforms().findLocation("inBoundDim");
        _drawItemCellLocLoc = program->getUniforms().findLocation("inCellLocation");

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
        auto vs = gpu::Shader::createVertex(std::string(drawItemStatus_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawItemStatus_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("iconStatusMap"), 0));
        gpu::Shader::makeProgram(*program, slotBindings);

        _drawItemStatusPosLoc = program->getUniforms().findLocation("inBoundPos");
        _drawItemStatusDimLoc = program->getUniforms().findLocation("inBoundDim");
        _drawItemStatusValue0Loc = program->getUniforms().findLocation("inStatus0");
        _drawItemStatusValue1Loc = program->getUniforms().findLocation("inStatus1");

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

void DrawStatus::run(const SceneContextPointer& sceneContext,
                     const RenderContextPointer& renderContext,
                     const ItemBounds& inItems) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& scene = sceneContext->_scene;
    const int NUM_STATUS_VEC4_PER_ITEM = 2;
    const int VEC4_LENGTH = 4;

    // FIrst thing, we collect the bound and the status for all the items we want to render
    int nbItems = 0;
    {
        _itemBounds.resize(inItems.size());
        _itemStatus.resize(inItems.size());
        _itemCells.resize(inItems.size());

//        AABox* itemAABox = reinterpret_cast<AABox*> (_itemBounds->editData());
//        glm::ivec4* itemStatus = reinterpret_cast<glm::ivec4*> (_itemStatus->editData());
//        Octree::Location* itemCell = reinterpret_cast<Octree::Location*> (_itemCells->editData());
        for (size_t i = 0; i < inItems.size(); ++i) {
            const auto& item = inItems[i];
            if (!item.bound.isInvalid()) {
                if (!item.bound.isNull()) {
                    _itemBounds[i] = item.bound;
                } else {
                    _itemBounds[i].setBox(item.bound.getCorner(), 0.1f);
                }
                

                auto& itemScene = scene->getItem(item.id);
                _itemCells[i] = scene->getSpatialTree().getCellLocation(itemScene.getCell());

                auto itemStatusPointer = itemScene.getStatus();
                if (itemStatusPointer) {
                    // Query the current status values, this is where the statusGetter lambda get called
                    auto&& currentStatusValues = itemStatusPointer->getCurrentValues();
                    int valueNum = 0;
                    for (int vec4Num = 0; vec4Num < NUM_STATUS_VEC4_PER_ITEM; vec4Num++) {
                        auto& value = (vec4Num ? _itemStatus[i].first : _itemStatus[i].second);
                        value = glm::ivec4(Item::Status::Value::INVALID.getPackedData());
                        for (int component = 0; component < VEC4_LENGTH; component++) {
                            valueNum = vec4Num * VEC4_LENGTH + component;
                            if (valueNum < (int)currentStatusValues.size()) {
                                value[component] = currentStatusValues[valueNum].getPackedData();
                            }
                        }
                    }
                } else {
                    _itemStatus[i].first = _itemStatus[i].second = glm::ivec4(Item::Status::Value::INVALID.getPackedData());
                }
                nbItems++;
            }
        }
    }

    if (nbItems == 0) {
        return;
    }

    // Allright, something to render let's do it
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setViewportTransform(args->_viewport);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat, true);
        batch.setModelTransform(Transform());

        // bind the one gpu::Pipeline we need
        batch.setPipeline(getDrawItemBoundsPipeline());

        //AABox* itemAABox = reinterpret_cast<AABox*> (_itemBounds->editData());
        //glm::ivec4* itemStatus = reinterpret_cast<glm::ivec4*> (_itemStatus->editData());
        //Octree::Location* itemCell = reinterpret_cast<Octree::Location*> (_itemCells->editData());

        const unsigned int VEC3_ADRESS_OFFSET = 3;

        if (_showDisplay) {
            for (int i = 0; i < nbItems; i++) {
                batch._glUniform3fv(_drawItemBoundPosLoc, 1, (const float*)&(_itemBounds[i]));
                batch._glUniform3fv(_drawItemBoundDimLoc, 1, ((const float*)&(_itemBounds[i])) + VEC3_ADRESS_OFFSET);

                glm::ivec4 cellLocation(_itemCells[i].pos, _itemCells[i].depth);
                batch._glUniform4iv(_drawItemCellLocLoc, 1, ((const int*)(&cellLocation)));
                batch.draw(gpu::LINES, 24, 0);
            }
        }

        batch.setResourceTexture(0, gpu::TextureView(getStatusIconMap(), 0));

        batch.setPipeline(getDrawItemStatusPipeline());

        if (_showNetwork) {
            for (int i = 0; i < nbItems; i++) {
                batch._glUniform3fv(_drawItemStatusPosLoc, 1, (const float*)&(_itemBounds[i]));
                batch._glUniform3fv(_drawItemStatusDimLoc, 1, ((const float*)&(_itemBounds[i])) + VEC3_ADRESS_OFFSET);
                batch._glUniform4iv(_drawItemStatusValue0Loc, 1, (const int*)&(_itemStatus[i].first));
                batch._glUniform4iv(_drawItemStatusValue1Loc, 1, (const int*)&(_itemStatus[i].second));
                batch.draw(gpu::TRIANGLES, 24 * NUM_STATUS_VEC4_PER_ITEM, 0);
            }
        }
        batch.setResourceTexture(0, 0);
    });
}
