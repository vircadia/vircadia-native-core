//
//  HitEffect.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 7/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL


#include <glm/gtc/random.hpp>

#include <DependencyManager.h>
#include <PathUtils.h>
#include <SharedUtil.h>

#include "AbstractViewStateInterface.h"
#include "HitEffect.h"

#include "TextureCache.h"
#include "DependencyManager.h"
#include "ViewFrustum.h"
#include "GeometryCache.h"

#include <gpu/Context.h>

#include "hit_effect_vert.h"
#include "hit_effect_frag.h"


HitEffect::HitEffect() {
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

HitEffect::~HitEffect() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (_geometryId && geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
}

const gpu::PipelinePointer& HitEffect::getHitEffectPipeline() {
    if (!_hitEffectPipeline) {
        auto vs = gpu::Shader::createVertex(std::string(hit_effect_vert));
        auto ps = gpu::Shader::createPixel(std::string(hit_effect_frag));
        gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
        
        
        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);
        

        gpu::StatePointer state = gpu::StatePointer(new gpu::State());
        
        state->setDepthTest(false, false, gpu::LESS_EQUAL);
        
        // Blend on transparent
        state->setBlendFunction(true,
                                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA);
    
        // Good to go add the brand new pipeline
        _hitEffectPipeline = gpu::Pipeline::create(program, state);
    }
    return _hitEffectPipeline;
}

void HitEffect::run(const render::SceneContextPointer& sceneContext, const render::RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        batch.setPipeline(getHitEffectPipeline());

        static const glm::vec4 color(0.0f, 0.0f, 0.0f, 1.0f);
        static const glm::vec2 bottomLeft(-1.0f, -1.0f);
        static const glm::vec2 topRight(1.0f, 1.0f);
        DependencyManager::get<GeometryCache>()->renderQuad(batch, bottomLeft, topRight, color, _geometryId);
    });
}

