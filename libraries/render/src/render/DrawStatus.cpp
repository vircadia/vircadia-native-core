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

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/GPUConfig.h>
#include <gpu/GPULogging.h>

#include <ViewFrustum.h>
#include <RenderArgs.h>

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

        _drawItemBoundPosLoc = program->getUniforms().findLocation("inBoundPos");
        _drawItemBoundDimLoc = program->getUniforms().findLocation("inBoundDim");

        auto state = std::make_shared<gpu::State>();

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

        _drawItemStatusPosLoc = program->getUniforms().findLocation("inBoundPos");
        _drawItemStatusDimLoc = program->getUniforms().findLocation("inBoundDim");
        _drawItemStatusValueLoc = program->getUniforms().findLocation("inStatus");

        auto state = std::make_shared<gpu::State>();

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
    auto& scene = sceneContext->_scene;

    // FIrst thing, we collect the bound and the status for all the items we want to render
    int nbItems = 0;
    {
        if (!_itemBounds) {
            _itemBounds = std::make_shared<gpu::Buffer>();
        }
        if (!_itemStatus) {
            _itemStatus = std::make_shared<gpu::Buffer>();;
        }

        _itemBounds->resize((inItems.size() * sizeof(AABox)));
        _itemStatus->resize((inItems.size() * sizeof(glm::vec4)));
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
                (*itemStatus) = itemScene.getStatusPackedValues();

                nbItems++;
                itemAABox++;
                itemStatus++;
            }
        }
    }

    if (nbItems == 0) {
        return;
    }

    // Allright, something to render let's do it
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
    batch.setModelTransform(Transform());

    // bind the one gpu::Pipeline we need
    batch.setPipeline(getDrawItemBoundsPipeline());

    AABox* itemAABox = reinterpret_cast<AABox*> (_itemBounds->editData());
    glm::ivec4* itemStatus = reinterpret_cast<glm::ivec4*> (_itemStatus->editData());

    const unsigned int VEC3_ADRESS_OFFSET = 3;

    for (int i = 0; i < nbItems; i++) {
        batch._glUniform3fv(_drawItemBoundPosLoc, 1, (const GLfloat*) (itemAABox + i));
        batch._glUniform3fv(_drawItemBoundDimLoc, 1, ((const GLfloat*) (itemAABox + i)) + VEC3_ADRESS_OFFSET);

        batch.draw(gpu::LINES, 24, 0);
    }

    batch.setPipeline(getDrawItemStatusPipeline());
    for (int i = 0; i < nbItems; i++) {
        batch._glUniform3fv(_drawItemStatusPosLoc, 1, (const GLfloat*) (itemAABox + i));
        batch._glUniform3fv(_drawItemStatusDimLoc, 1, ((const GLfloat*) (itemAABox + i)) + VEC3_ADRESS_OFFSET);
        batch._glUniform4iv(_drawItemStatusValueLoc, 1, (const GLint*) (itemStatus + i));

        batch.draw(gpu::TRIANGLES, 24, 0);
    }

    // Before rendering the batch make sure we re in sync with gl state
    args->_context->syncCache();
    renderContext->args->_context->syncCache();
    args->_context->render((batch));
}