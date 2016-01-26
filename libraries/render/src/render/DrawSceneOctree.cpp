//
//  DrawSceneOctree.cpp
//  render/src/render
//
//  Created by Sam Gateau on 1/25/16.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DrawSceneOctree.h"

#include <algorithm>
#include <assert.h>

#include <PerfStat.h>
#include <ViewFrustum.h>
#include <RenderArgs.h>

#include <gpu/Context.h>

#include "drawCellBounds_vert.h"
#include "drawCellBounds_frag.h"

using namespace render;

const int DrawSceneOctree_CellsSlot = 0;
const int DrawSceneOctree_OctreeInfoSlot = 1;

const gpu::PipelinePointer DrawSceneOctree::getDrawCellBoundsPipeline() {
    if (!_drawCellBoundsPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(drawCellBounds_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawCellBounds_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("cellsBuffer"), DrawSceneOctree_CellsSlot));
        slotBindings.insert(gpu::Shader::Binding(std::string("octreeInfoBuffer"), DrawSceneOctree_OctreeInfoSlot));
        gpu::Shader::makeProgram(*program, slotBindings);
        
        auto state = std::make_shared<gpu::State>();

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Good to go add the brand new pipeline
        _drawCellBoundsPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawCellBoundsPipeline;
}

void DrawSceneOctree::run(const SceneContextPointer& sceneContext,
                     const RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);
    RenderArgs* args = renderContext->getArgs();
    auto& scene = sceneContext->_scene;
    const int NUM_STATUS_VEC4_PER_ITEM = 2;
    const int VEC4_LENGTH = 4;

    // FIrst thing, we update the local buffers with the values coming from Scene octree
    int nbCells = 0;
    {
        if (!_cells) {
            _cells = std::make_shared<gpu::Buffer>();
        }
        if (!_octreeInfo) {
            _octreeInfo = std::make_shared<gpu::Buffer>();;
        }
        /*
        _cells->resize((inItems.size() * sizeof(AABox)));
        _itemStatus->resize((inItems.size() * NUM_STATUS_VEC4_PER_ITEM * sizeof(glm::vec4)));
        AABox* itemAABox = reinterpret_cast<AABox*> (_itemBounds->editData());
        glm::ivec4* itemStatus = reinterpret_cast<glm::ivec4*> (_itemStatus->editData());
        for (auto& item : inItems) {
            if (!item.bounds.isInvalid()) {
                if (!item.bounds.isNull()) {
                    (*itemAABox) = item.bounds;
                } else {
                    (*itemAABox).setBox(item.bounds.getCorner(), 0.1f);
                }
                auto& itemScene = scene->getItem(item.id);

                auto itemStatusPointer = itemScene.getStatus();
                if (itemStatusPointer) {
                    // Query the current status values, this is where the statusGetter lambda get called
                    auto&& currentStatusValues = itemStatusPointer->getCurrentValues();
                    int valueNum = 0;
                    for (int vec4Num = 0; vec4Num < NUM_STATUS_VEC4_PER_ITEM; vec4Num++) {
                        (*itemStatus) = glm::ivec4(Item::Status::Value::INVALID.getPackedData());
                        for (int component = 0; component < VEC4_LENGTH; component++) {
                            valueNum = vec4Num * VEC4_LENGTH + component;
                            if (valueNum < (int)currentStatusValues.size()) {
                                (*itemStatus)[component] = currentStatusValues[valueNum].getPackedData();
                            }
                        }
                        itemStatus++;
                    }
                } else {
                    (*itemStatus) = glm::ivec4(Item::Status::Value::INVALID.getPackedData());
                    itemStatus++;
                    (*itemStatus) = glm::ivec4(Item::Status::Value::INVALID.getPackedData());
                    itemStatus++;
                }

                nbItems++;
                itemAABox++;
            }
        }*/
    }

    if (nbCells == 0) {
        return;
    }

    // Allright, something to render let's do it
   /* gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        batch.setViewportTransform(args->_viewport);

        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // bind the one gpu::Pipeline we need
        batch.setPipeline(getDrawCellBoundsPipeline());

        AABox* itemAABox = reinterpret_cast<AABox*> (_itemBounds->editData());
        glm::ivec4* itemStatus = reinterpret_cast<glm::ivec4*> (_itemStatus->editData());

        const unsigned int VEC3_ADRESS_OFFSET = 3;

        if ((renderContext->getDrawStatus() & showDisplayStatusFlag) > 0) {
            for (int i = 0; i < nbCells; i++) {
                batch._glUniform3fv(_drawItemBoundPosLoc, 1, (const float*) (itemAABox + i));
                batch._glUniform3fv(_drawItemBoundDimLoc, 1, ((const float*) (itemAABox + i)) + VEC3_ADRESS_OFFSET);

                batch.draw(gpu::LINES, 24, 0);
            }
        }
    });*/
}
