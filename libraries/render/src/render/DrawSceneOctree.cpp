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

        _drawCellLocationLoc = program->getUniforms().findLocation("inCellLocation");

        auto state = std::make_shared<gpu::State>();

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,  gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        // Good to go add the brand new pipeline
        _drawCellBoundsPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawCellBoundsPipeline;
}


void DrawSceneOctree::configure(const Config& config) {
    _showVisibleCells = config.showVisibleCells;

    _justFrozeFrustum = (config.freezeFrustum && !_freezeFrustum);
    _freezeFrustum = config.freezeFrustum;
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

    auto queryFrustum = *args->_viewFrustum;
    if (_freezeFrustum) {
        if (_justFrozeFrustum) {
            _justFrozeFrustum = false;
            _frozenFrutstum = *args->_viewFrustum;
        }
        queryFrustum = _frozenFrutstum;
    }

    // Try that:
    render::ItemIDs itemsCell;
    scene->getSpatialTree().select(itemsCell, queryFrustum);


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

     /*   const auto& inCells = scene->getSpatialTree()._cells;

        for (const auto& cell: inCells ) {
       */
        
        for (const auto& cellID : itemsCell) {
            auto cell = scene->getSpatialTree().getConcreteCell(cellID);

            auto cellLoc = cell.getlocation();

            glm::ivec4 cellLocation(cellLoc.pos.x, cellLoc.pos.y, cellLoc.pos.z, cellLoc.depth);
            if (cell.isBrickEmpty() || !cell.hasBrick()) {
            //    cellLocation.w *= -1;
            }

            batch._glUniform4iv(_drawCellLocationLoc, 1, ((const int*)(&cellLocation)));

            batch.draw(gpu::LINES, 24, 0);
        }
    });
}
