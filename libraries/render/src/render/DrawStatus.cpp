//
//  DrawStatus.cpp
//  render/src/render
//
//  Created by Niraj Venkat on 5/21/15.
//  Copyright 20154 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <algorithm>
#include <assert.h>

#include "DrawStatus.h"

#include <PerfStat.h>
#include "gpu/GPULogging.h"


#include "gpu/Batch.h"
#include "gpu/Context.h"

#include "ViewFrustum.h"
#include "RenderArgs.h"

#include "drawItemBounds_vert.h"
#include "drawItemBounds_frag.h"
#include "drawItemStatus_vert.h"
#include "drawItemStatus_frag.h"

using namespace render;



const gpu::PipelinePointer& DrawStatus::getDrawItemBoundsPipeline() {
    if (!_drawItemBoundsPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(drawItemBounds_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(drawItemBounds_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(true, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Good to go add the brand new pipeline
        _drawItemBoundsPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _drawItemBoundsPipeline;
}

const gpu::PipelinePointer& DrawStatus::getDrawItemStatusPipeline() {
    if (!_drawItemStatusPipeline) {
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(drawItemStatus_vert)));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(drawItemStatus_frag)));
        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));

        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());

        state->setDepthTest(false, false, gpu::LESS_EQUAL);

        // Blend on transparent
        state->setBlendFunction(true,
            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
            gpu::State::DEST_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ZERO);

        // Good to go add the brand new pipeline
        _drawItemStatusPipeline.reset(gpu::Pipeline::create(program, state));
    }
    return _drawItemStatusPipeline;
}

void DrawStatus::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const ItemIDsBounds& inItems) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;

    gpu::Batch batch;

    glm::mat4 projMat;
    Transform viewMat;
    args->_viewFrustum->evalProjectionMatrix(projMat);
    args->_viewFrustum->evalViewTransform(viewMat);
    if (args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
        viewMat.postScale(glm::vec3(-1.0f, 1.0f, 1.0f));
    }
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);

    
    // batch.setModelTransform(Transform());
    // bind the unit cube geometry

    // bind the one gpu::Pipeline we need
    batch.setPipeline(getDrawItemBoundsPipeline());

    for (auto& item : inItems) {
        if (!item.bounds.isInvalid()) {
            Transform model;
            model.setTranslation(item.bounds.getCorner());
            if (!item.bounds.isNull()) {
                model.setScale(item.bounds.getDimensions());
            }

            batch.setModelTransform(model);
            batch.draw(gpu::LINE_STRIP, 13, 0);
        }
    }

    batch.setPipeline(getDrawItemStatusPipeline());

    for (auto& item : inItems) {
        if (!item.bounds.isInvalid()) {
            Transform model;
            model.setTranslation(item.bounds.getCorner());
            if (!item.bounds.isNull()) {
                model.setScale(item.bounds.getDimensions());
            }

            batch.setModelTransform(model);
            batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
        }
    }

    // Before rendering the batch make sure we re in sync with gl state
    args->_context->syncCache();
    renderContext->args->_context->syncCache();
    args->_context->render((batch));
    args->_batch = nullptr;

}