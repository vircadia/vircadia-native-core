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

#include <QString>

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <render/Scene.h>
#include <ViewFrustum.h>

#include "GeometryCache.h"
#include "FramebufferCache.h"

#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"

using namespace render;

enum Slots {
    Diffuse = 0,
    Normal,
    Specular,
    Depth,
    Lighting,
    
    NUM_SLOTS
};
static const std::array<std::string, Slots::NUM_SLOTS> SLOT_NAMES {{
    "diffuseMap",
    "normalMap",
    "specularMap",
    "depthMap",
    "lightingMap"
}};

static const std::string COMPUTE_PLACEHOLDER { "/*COMPUTE_PLACEHOLDER*/" }; // required
static const std::string FUNCTIONS_PLACEHOLDER { "/*FUNCTIONS_PLACEHOLDER*/" }; // optional

std::string DebugDeferredBuffer::getCode(Modes mode) {
    switch (mode) {
        case DiffuseMode: {
            QString code = "return vec4(pow(texture(%1, uv).xyz, vec3(1.0 / 2.2)), 1.0);";
            return code.arg(SLOT_NAMES[Diffuse].c_str()).toStdString();
        }
        case AlphaMode: {
            QString code = "return vec4(vec3(texture(%1, uv).a), 1.0);";
            return code.arg(SLOT_NAMES[Diffuse].c_str()).toStdString();
        }
        case SpecularMode: {
            QString code = "return vec4(texture(%1, uv).xyz, 1.0);";
            return code.arg(SLOT_NAMES[Specular].c_str()).toStdString();
        }
        case RoughnessMode: {
            QString code = "return vec4(vec3(texture(%1, uv).a), 1.0);";
            return code.arg(SLOT_NAMES[Specular].c_str()).toStdString();
        }
        case NormalMode: {
            QString code = "return vec4(texture(%1, uv).xyz, 1.0);";
            return code.arg(SLOT_NAMES[Normal].c_str()).toStdString();
        }
        case DepthMode: {
            QString code = "return vec4(vec3(texture(%1, uv).x), 1.0);";
            return code.arg(SLOT_NAMES[Depth].c_str()).toStdString();
        }
        case LightingMode: {
            QString code = "return vec4(pow(texture(%1, uv).xyz, vec3(1.0 / 2.2)), 1.0);";
            return code.arg(SLOT_NAMES[Lighting].c_str()).toStdString();
        }
        case CustomMode:
            return std::string("return vec4(1.0);");
        case NUM_MODES:
            Q_UNIMPLEMENTED();
            return std::string("return vec4(1.0);");
    }
}

const gpu::PipelinePointer& DebugDeferredBuffer::getPipeline(Modes mode) {
    if (!_pipelines[mode]) {
        std::string fragmentShader = debug_deferred_buffer_frag;
        fragmentShader.replace(fragmentShader.find(COMPUTE_PLACEHOLDER), COMPUTE_PLACEHOLDER.size(),
                               getCode(mode));
        
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex({ debug_deferred_buffer_vert }));
        auto ps = gpu::ShaderPointer(gpu::Shader::createPixel(fragmentShader));
        auto program = gpu::ShaderPointer(gpu::Shader::createProgram(vs, ps));
        
        gpu::Shader::BindingSet slotBindings;
        for (int slot = 0; slot < NUM_SLOTS; ++slot) {
            slotBindings.insert(gpu::Shader::Binding(SLOT_NAMES[slot], slot));
        }
        gpu::Shader::makeProgram(*program, slotBindings);
        
        // Good to go add the brand new pipeline
        _pipelines[mode] = gpu::Pipeline::create(program, std::make_shared<gpu::State>());
    }
    return _pipelines[mode];
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
        
        batch.setPipeline(getPipeline(Modes(renderContext->_deferredDebugMode)));
        
        batch.setResourceTexture(Diffuse, framebufferCache->getDeferredColorTexture());
        batch.setResourceTexture(Normal, framebufferCache->getDeferredNormalTexture());
        batch.setResourceTexture(Specular, framebufferCache->getDeferredSpecularTexture());
        batch.setResourceTexture(Depth, framebufferCache->getPrimaryDepthTexture());
        batch.setResourceTexture(Lighting, framebufferCache->getLightingTexture());
        
        glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec2 bottomLeft(renderContext->_deferredDebugSize.x, renderContext->_deferredDebugSize.y);
        glm::vec2 topRight(renderContext->_deferredDebugSize.z, renderContext->_deferredDebugSize.w);
        geometryBuffer->renderQuad(batch, bottomLeft, topRight, color);
    });
}