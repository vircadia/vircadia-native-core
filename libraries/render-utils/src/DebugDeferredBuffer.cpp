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

#include <QFile>

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
    Lighting
};

static std::string getFileContent(std::string fileName, std::string defaultContent = std::string()) {
    QFile customFile(QString::fromStdString(fileName));
    if (customFile.open(QIODevice::ReadOnly)) {
        return customFile.readAll().toStdString();
    }
    return defaultContent;
}

std::string DebugDeferredBuffer::getShaderSourceCode(Modes mode) {
    switch (mode) {
        case DiffuseMode:
            return "vec4 getFragmentColor() { return vec4(pow(texture(diffuseMap, uv).xyz, vec3(1.0 / 2.2)), 1.0); }";
        case AlphaMode:
            return "vec4 getFragmentColor() { return vec4(vec3(texture(diffuseMap, uv).a), 1.0); }";
        case SpecularMode:
            return "vec4 getFragmentColor() { return vec4(texture(specularMap, uv).xyz, 1.0); }";
        case RoughnessMode:
            return "vec4 getFragmentColor() { return vec4(vec3(texture(specularMap, uv).a), 1.0); }";
        case NormalMode:
            return "vec4 getFragmentColor() { return vec4(texture(normalMap, uv).xyz, 1.0); }";
        case DepthMode:
            return "vec4 getFragmentColor() { return vec4(vec3(texture(depthMap, uv).x), 1.0); }";
        case LightingMode:
            return "vec4 getFragmentColor() { return vec4(pow(texture(lightingMap, uv).xyz, vec3(1.0 / 2.2)), 1.0); }";
        case CustomMode:
            return getFileContent(CUSTOM_FILE, "vec4 getFragmentColor() { return vec4(1.0); }");
    }
}

bool DebugDeferredBuffer::pipelineNeedsUpdate(Modes mode) const {
    if (mode != CustomMode) {
        return !_pipelines[mode];
    }
    
    return true;
}

const gpu::PipelinePointer& DebugDeferredBuffer::getPipeline(Modes mode) {
    if (pipelineNeedsUpdate(mode)) {
        static const std::string VERTEX_SHADER { debug_deferred_buffer_vert };
        static const std::string FRAGMENT_SHADER { debug_deferred_buffer_frag };
        static const std::string SOURCE_PLACEHOLDER { "//SOURCE_PLACEHOLDER" };
        static const auto SOURCE_PLACEHOLDER_INDEX = FRAGMENT_SHADER.find(SOURCE_PLACEHOLDER);
        Q_ASSERT_X(SOURCE_PLACEHOLDER_INDEX != std::string::npos, Q_FUNC_INFO, "Could not find source placeholder");
        
        auto bakedFragmentShader = FRAGMENT_SHADER;
        bakedFragmentShader.replace(SOURCE_PLACEHOLDER_INDEX, SOURCE_PLACEHOLDER.size(),
                                    getShaderSourceCode(mode));
        
        const auto vs = gpu::Shader::createVertex(VERTEX_SHADER);
        const auto ps = gpu::Shader::createPixel(bakedFragmentShader);
        const auto program = gpu::Shader::createProgram(vs, ps);
        
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("diffuseMap", Diffuse));
        slotBindings.insert(gpu::Shader::Binding("normalMap", Normal));
        slotBindings.insert(gpu::Shader::Binding("specularMap", Specular));
        slotBindings.insert(gpu::Shader::Binding("depthMap", Depth));
        slotBindings.insert(gpu::Shader::Binding("lightingMap", Lighting));
        gpu::Shader::makeProgram(*program, slotBindings);
        
        // Good to go add the brand new pipeline
        _pipelines[mode] = gpu::Pipeline::create(program, std::make_shared<gpu::State>());
    }
    return _pipelines[mode];
}


void DebugDeferredBuffer::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->args);
    assert(renderContext->args->_viewFrustum);
    const RenderArgs* args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        const auto geometryBuffer = DependencyManager::get<GeometryCache>();
        const auto framebufferCache = DependencyManager::get<FramebufferCache>();
        
        
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
        
        const glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
        const glm::vec2 bottomLeft(renderContext->_deferredDebugSize.x, renderContext->_deferredDebugSize.y);
        const glm::vec2 topRight(renderContext->_deferredDebugSize.z, renderContext->_deferredDebugSize.w);
        geometryBuffer->renderQuad(batch, bottomLeft, topRight, color);
    });
}