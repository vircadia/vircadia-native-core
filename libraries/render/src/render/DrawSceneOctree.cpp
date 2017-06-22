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

#include <OctreeUtils.h>
#include <PerfStat.h>

#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "Args.h"

#include "drawCellBounds_vert.h"
#include "drawCellBounds_frag.h"
#include "drawLODReticle_frag.h"

#include "drawItemBounds_vert.h"
#include "drawItemBounds_frag.h"

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

const gpu::PipelinePointer DrawSceneOctree::getDrawLODReticlePipeline() {
    if (!_drawLODReticlePipeline) {
        auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
        auto ps = gpu::Shader::createPixel(std::string(drawLODReticle_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

       // _drawCellLocationLoc = program->getUniforms().findLocation("inCellLocation");

        auto state = std::make_shared<gpu::State>();

        // Blend on transparent
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        // Good to go add the brand new pipeline
        _drawLODReticlePipeline = gpu::Pipeline::create(program, state);
    }
    return _drawLODReticlePipeline;
}

void DrawSceneOctree::configure(const Config& config) {
    _showVisibleCells = config.showVisibleCells;
    _showEmptyCells = config.showEmptyCells;
}


void DrawSceneOctree::run(const RenderContextPointer& renderContext, const ItemSpatialTree::ItemSelection& inSelection) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& scene = renderContext->_scene;

    std::static_pointer_cast<Config>(renderContext->jobConfig)->numAllocatedCells = (int)scene->getSpatialTree().getNumAllocatedCells();
    std::static_pointer_cast<Config>(renderContext->jobConfig)->numFreeCells = (int)scene->getSpatialTree().getNumFreeCells();


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
        batch.setPipeline(getDrawCellBoundsPipeline());

        if (_showVisibleCells) {

            for (const auto& cellID : inSelection.cellSelection.insideCells) {
                auto cell = scene->getSpatialTree().getConcreteCell(cellID);
                auto cellLoc = cell.getlocation();
                glm::ivec4 cellLocation(cellLoc.pos.x, cellLoc.pos.y, cellLoc.pos.z, cellLoc.depth);

                bool doDraw = true;
                if (cell.isBrickEmpty() || !cell.hasBrick()) {
                    if (!_showEmptyCells) {
                        doDraw = false;
                    }
                    cellLocation.w *= -1;
                }
                if (doDraw) {
                    batch._glUniform4iv(_drawCellLocationLoc, 1, ((const int*)(&cellLocation)));
                    batch.draw(gpu::LINES, 24, 0);
                }
            }

            for (const auto& cellID : inSelection.cellSelection.partialCells) {
                auto cell = scene->getSpatialTree().getConcreteCell(cellID);
                auto cellLoc = cell.getlocation();
                glm::ivec4 cellLocation(cellLoc.pos.x, cellLoc.pos.y, cellLoc.pos.z, cellLoc.depth);

                bool doDraw = true;
                if (cell.isBrickEmpty() || !cell.hasBrick()) {
                    if (!_showEmptyCells) {
                        doDraw = false;
                    }
                    cellLocation.w *= -1;
                }
                if (doDraw) {
                    batch._glUniform4iv(_drawCellLocationLoc, 1, ((const int*)(&cellLocation)));
                    batch.draw(gpu::LINES, 24, 0);
                }
            }
        }
        // Draw the LOD Reticle
        {
            float angle = glm::degrees(getAccuracyAngle(args->_sizeScale, args->_boundaryLevelAdjust));
            Transform crosshairModel;
            crosshairModel.setTranslation(glm::vec3(0.0, 0.0, -1000.0));
            crosshairModel.setScale(1000.0 * tan(glm::radians(angle))); // Scaling at the actual tan of the lod angle => Multiplied by TWO
            batch.resetViewTransform();
            batch.setModelTransform(crosshairModel);
            batch.setPipeline(getDrawLODReticlePipeline());
            batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
        }
    });
}

const gpu::PipelinePointer DrawItemSelection::getDrawItemBoundPipeline() {
    if (!_drawItemBoundPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(drawItemBounds_vert));
        auto ps = gpu::Shader::createPixel(std::string(drawItemBounds_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        _drawItemBoundPosLoc = program->getUniforms().findLocation("inBoundPos");
        _drawItemBoundDimLoc = program->getUniforms().findLocation("inBoundDim");

        _drawCellLocationLoc = program->getUniforms().findLocation("inCellLocation");

        auto state = std::make_shared<gpu::State>();

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true, gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);

        // Good to go add the brand new pipeline
        _drawItemBoundPipeline = gpu::Pipeline::create(program, state);
    }
    return _drawItemBoundPipeline;
}

void DrawItemSelection::configure(const Config& config) {
    _showInsideItems = config.showInsideItems;
    _showInsideSubcellItems = config.showInsideSubcellItems;
    _showPartialItems = config.showPartialItems;
    _showPartialSubcellItems = config.showPartialSubcellItems;
}


void DrawItemSelection::run(const RenderContextPointer& renderContext, const ItemSpatialTree::ItemSelection& inSelection) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;
    auto& scene = renderContext->_scene;

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
        batch.setPipeline(getDrawItemBoundPipeline());

        if (_showInsideItems) {
            for (const auto& itemID : inSelection.insideItems) {
                auto& item = scene->getItem(itemID);
                auto itemBound = item.getBound();
                auto itemCell = scene->getSpatialTree().getCellLocation(item.getCell());
                glm::ivec4 cellLocation(0, 0, 0, itemCell.depth);

                batch._glUniform4iv(_drawCellLocationLoc, 1, ((const int*)(&cellLocation)));
                batch._glUniform3fv(_drawItemBoundPosLoc, 1, (const float*)(&itemBound.getCorner()));
                batch._glUniform3fv(_drawItemBoundDimLoc, 1, (const float*)(&itemBound.getScale()));

                batch.draw(gpu::LINES, 24, 0);
            }
        }

        if (_showInsideSubcellItems) {
            for (const auto& itemID : inSelection.insideSubcellItems) {
                auto& item = scene->getItem(itemID);
                auto itemBound = item.getBound();
                auto itemCell = scene->getSpatialTree().getCellLocation(item.getCell());
                glm::ivec4 cellLocation(0, 0, 1, itemCell.depth);

                batch._glUniform4iv(_drawCellLocationLoc, 1, ((const int*)(&cellLocation)));
                batch._glUniform3fv(_drawItemBoundPosLoc, 1, (const float*)(&itemBound.getCorner()));
                batch._glUniform3fv(_drawItemBoundDimLoc, 1, (const float*)(&itemBound.getScale()));

                batch.draw(gpu::LINES, 24, 0);
            }
        }

        if (_showPartialItems) {
            for (const auto& itemID : inSelection.partialItems) {
                auto& item = scene->getItem(itemID);
                auto itemBound = item.getBound();
                auto itemCell = scene->getSpatialTree().getCellLocation(item.getCell());
                glm::ivec4 cellLocation(0, 0, 0, itemCell.depth);

                batch._glUniform4iv(_drawCellLocationLoc, 1, ((const int*)(&cellLocation)));
                batch._glUniform3fv(_drawItemBoundPosLoc, 1, (const float*)(&itemBound.getCorner()));
                batch._glUniform3fv(_drawItemBoundDimLoc, 1, (const float*)(&itemBound.getScale()));

                batch.draw(gpu::LINES, 24, 0);
            }
        }

        if (_showPartialSubcellItems) {
            for (const auto& itemID : inSelection.partialSubcellItems) {
                auto& item = scene->getItem(itemID);
                auto itemBound = item.getBound();
                auto itemCell = scene->getSpatialTree().getCellLocation(item.getCell());
                glm::ivec4 cellLocation(0, 0, 1, itemCell.depth);
                batch._glUniform4iv(_drawCellLocationLoc, 1, ((const int*)(&cellLocation)));
                batch._glUniform3fv(_drawItemBoundPosLoc, 1, (const float*)(&itemBound.getCorner()));
                batch._glUniform3fv(_drawItemBoundDimLoc, 1, (const float*)(&itemBound.getScale()));

                batch.draw(gpu::LINES, 24, 0);
            }
        }
    });
}
