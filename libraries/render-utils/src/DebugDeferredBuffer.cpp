//
//  DebugDeferredBuffer.cpp
//  libraries/render-utils/src
//
//  Created by Clement on 12/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DebugDeferredBuffer.h"

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <render/Scene.h>
#include <ViewFrustum.h>

#include "GeometryCache.h"
#include "FramebufferCache.h"

#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"

using namespace render;

static const std::string PLACEHOLDER { "DEBUG_PLACEHOLDER" };
static const std::array<std::string, DebugDeferredBuffer::NUM_SLOTS> SLOT_NAMES {{
    "diffuseMap",
    "normalMap",
    "specularMap",
    "depthMap",
    "lightingMap"
}};

std::string getCode(int slot) {
    return std::string("return texture(").append(SLOT_NAMES[slot]).append(", uv);");
}

const gpu::PipelinePointer& DebugDeferredBuffer::getPipeline(int slot) {
    if (!_pipelines[slot]) {
        std::string fragmentShader = debug_deferred_buffer_frag;
        fragmentShader.replace(fragmentShader.find(PLACEHOLDER), PLACEHOLDER.size(), getCode(slot));
        
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex({ debug_deferred_buffer_vert }));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(fragmentShader));
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));
        
        gpu::Shader::BindingSet slotBindings;
        for (int slot = 0; slot < NUM_SLOTS; ++slot) {
            slotBindings.insert(gpu::Shader::Binding(SLOT_NAMES[slot], slot));
        }
        gpu::Shader::makeProgram(*program, slotBindings);
        
        // Good to go add the brand new pipeline
        _pipelines[slot] = gpu::Pipeline::create(program, std::make_shared<gpu::State>());
    }
    return _pipelines[slot];
}


void DebugDeferredBuffer::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        auto geometryBuffer = DependencyManager::get<GeometryCache>();
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
        
        
        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());
        
        batch.setPipeline(getPipeline((DebugDeferredBufferSlot)(renderContext->_drawDebugDeferredBuffer - 1)));
        
        batch.setResourceTexture(Diffuse, framebufferCache->getDeferredColorTexture());
        batch.setResourceTexture(Normal, framebufferCache->getDeferredNormalTexture());
        batch.setResourceTexture(Specular, framebufferCache->getDeferredSpecularTexture());
        batch.setResourceTexture(Depth, framebufferCache->getPrimaryDepthTexture());
        batch.setResourceTexture(Lighting, framebufferCache->getLightingTexture());
        
        glm::vec4 color(0.0f, 0.0f, 1.0f, 1.0f);
        glm::vec2 bottomLeft(0.0f, -1.0f);
        glm::vec2 topRight(1.0f, 1.0f);
        geometryBuffer->renderQuad(batch, bottomLeft, topRight, color);
    });
}