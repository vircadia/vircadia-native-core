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


const gpu::PipelinePointer DrawSceneOctree::getDrawCellBoundsPipeline() {
    if (!_drawCellBoundsPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(drawCellBounds_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawCellBounds_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);
        
        _drawBoundPosLoc = program->getUniforms().findLocation("inBoundPos");
        _drawBoundDimLoc = program->getUniforms().findLocation("inBoundDim");

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
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;
    auto& scene = sceneContext->_scene;
    const int NUM_STATUS_VEC4_PER_ITEM = 2;
    const int VEC4_LENGTH = 4;

    // FIrst thing, we update the local buffers with the values coming from Scene octree
    int nbCells = 0;
    {
        if (!_cells) {
            _cells = std::make_shared<gpu::Buffer>();
        }
       /* if (!_octreeInfo) {
            _octreeInfo = std::make_shared<gpu::Buffer>();;
        }*/
        
        const auto& inCells = scene->getSpatialTree()._cells;
        _cells->resize(inCells.size() * sizeof(AABox));
        AABox* cellAABox = reinterpret_cast<AABox*> (_cells->editData());
        for (const auto& cell : inCells) {
            (*cellAABox) = scene->getSpatialTree().evalBound(cell.getlocation());
            nbCells++;
            cellAABox++;
        }
    }

    if (nbCells == 0) {
        return;
    }

    // Allright, something to render let's do it
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
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

        AABox* cellAABox = reinterpret_cast<AABox*> (_cells->editData());

        const unsigned int VEC3_ADRESS_OFFSET = 3;

        for (int i = 0; i < nbCells; i++) {
            batch._glUniform3fv(_drawBoundPosLoc, 1, (const float*) (cellAABox + i));
            batch._glUniform3fv(_drawBoundDimLoc, 1, ((const float*) (cellAABox + i)) + VEC3_ADRESS_OFFSET);

            batch.draw(gpu::LINES, 24, 0);
        }
    });
}
