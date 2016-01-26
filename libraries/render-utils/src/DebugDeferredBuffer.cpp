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
#include "DeferredLightingEffect.h"

#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"

using namespace render;

enum Slots {
    Diffuse = 0,
    Normal,
    Specular,
    Depth,
    Lighting,
    Shadow,
    Pyramid,
    AmbientOcclusion,
    AmbientOcclusionBlurred
};

static const std::string DEFAULT_DIFFUSE_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(pow(texture(diffuseMap, uv).xyz, vec3(1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_SPECULAR_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(texture(specularMap, uv).xyz, 1.0);"
    " }"
};
static const std::string DEFAULT_ROUGHNESS_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(specularMap, uv).a), 1.0);"
    " }"
};
static const std::string DEFAULT_NORMAL_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(normalize(texture(normalMap, uv).xyz * 2.0 - vec3(1.0)), 1.0);"
    " }"
};
static const std::string DEFAULT_DEPTH_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(depthMap, uv).x), 1.0);"
    " }"
};
static const std::string DEFAULT_LIGHTING_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(pow(texture(lightingMap, uv).xyz, vec3(1.0 / 2.2)), 1.0);"
    " }"
};
static const std::string DEFAULT_SHADOW_SHADER {
    "uniform sampler2DShadow shadowMap;"
    "vec4 getFragmentColor() {"
    "    for (int i = 255; i >= 0; --i) {"
    "        float depth = i / 255.0;"
    "        if (texture(shadowMap, vec3(uv, depth)) > 0.5) {"
    "            return vec4(vec3(depth), 1.0);"
    "        }"
    "    }"
    "    return vec4(vec3(0.0), 1.0);"
    " }"
};

static const std::string DEFAULT_PYRAMID_DEPTH_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(1.0 - texture(pyramidMap, uv).x * 0.01), 1.0);"
    //"    return vec4(vec3(1.0 - textureLod(pyramidMap, uv, 3).x * 0.01), 1.0);"
    " }"
};

static const std::string DEFAULT_AMBIENT_OCCLUSION_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(occlusionMap, uv).x), 1.0);"
    // When drawing color "    return vec4(vec3(texture(occlusionMap, uv).xyz), 1.0);"
    // when drawing normal "    return vec4(normalize(texture(occlusionMap, uv).xyz * 2.0 - vec3(1.0)), 1.0);"
    " }"
};
static const std::string DEFAULT_AMBIENT_OCCLUSION_BLURRED_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(occlusionBlurredMap, uv).x), 1.0);"
    " }"
};

static const std::string DEFAULT_CUSTOM_SHADER {
    "vec4 getFragmentColor() {"
    "    return vec4(1.0, 0.0, 0.0, 1.0);"
    " }"
};

static std::string getFileContent(std::string fileName, std::string defaultContent = std::string()) {
    QFile customFile(QString::fromStdString(fileName));
    if (customFile.open(QIODevice::ReadOnly)) {
        return customFile.readAll().toStdString();
    }
    qWarning() << "DebugDeferredBuffer::getFileContent(): Could not open"
               << QString::fromStdString(fileName);
    return defaultContent;
}

#include <QStandardPaths> // TODO REMOVE: Temporary until UI
DebugDeferredBuffer::DebugDeferredBuffer() {
    // TODO REMOVE: Temporary until UI
    static const auto DESKTOP_PATH = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    static const auto CUSTOM_FILE = DESKTOP_PATH.toStdString() + "/custom.slh";
    CustomPipeline pipeline;
    pipeline.info = QFileInfo(QString::fromStdString(CUSTOM_FILE));
    _customPipelines.emplace(CUSTOM_FILE, pipeline);
}

std::string DebugDeferredBuffer::getShaderSourceCode(Modes mode, std::string customFile) {
    switch (mode) {
        case DiffuseMode:
            return DEFAULT_DIFFUSE_SHADER;
        case SpecularMode:
            return DEFAULT_SPECULAR_SHADER;
        case RoughnessMode:
            return DEFAULT_ROUGHNESS_SHADER;
        case NormalMode:
            return DEFAULT_NORMAL_SHADER;
        case DepthMode:
            return DEFAULT_DEPTH_SHADER;
        case LightingMode:
            return DEFAULT_LIGHTING_SHADER;
        case ShadowMode:
            return DEFAULT_SHADOW_SHADER;
        case PyramidDepthMode:
            return DEFAULT_PYRAMID_DEPTH_SHADER;
        case AmbientOcclusionMode:
            return DEFAULT_AMBIENT_OCCLUSION_SHADER;
        case AmbientOcclusionBlurredMode:
            return DEFAULT_AMBIENT_OCCLUSION_BLURRED_SHADER;
        case CustomMode:
            return getFileContent(customFile, DEFAULT_CUSTOM_SHADER);
    }
    Q_UNREACHABLE();
    return std::string();
}

bool DebugDeferredBuffer::pipelineNeedsUpdate(Modes mode, std::string customFile) const {
    if (mode != CustomMode) {
        return !_pipelines[mode];
    }
    
    auto it = _customPipelines.find(customFile);
    if (it != _customPipelines.end() && it->second.pipeline) {
        auto& info = it->second.info;
        
        auto lastModified = info.lastModified();
        info.refresh();
        return lastModified != info.lastModified();
    }
    
    return true;
}

const gpu::PipelinePointer& DebugDeferredBuffer::getPipeline(Modes mode, std::string customFile) {
    if (pipelineNeedsUpdate(mode, customFile)) {
        static const std::string VERTEX_SHADER { debug_deferred_buffer_vert };
        static const std::string FRAGMENT_SHADER { debug_deferred_buffer_frag };
        static const std::string SOURCE_PLACEHOLDER { "//SOURCE_PLACEHOLDER" };
        static const auto SOURCE_PLACEHOLDER_INDEX = FRAGMENT_SHADER.find(SOURCE_PLACEHOLDER);
        Q_ASSERT_X(SOURCE_PLACEHOLDER_INDEX != std::string::npos, Q_FUNC_INFO,
                   "Could not find source placeholder");
        
        auto bakedFragmentShader = FRAGMENT_SHADER;
        bakedFragmentShader.replace(SOURCE_PLACEHOLDER_INDEX, SOURCE_PLACEHOLDER.size(),
                                    getShaderSourceCode(mode, customFile));
        
        static const auto vs = gpu::Shader::createVertex(VERTEX_SHADER);
        const auto ps = gpu::Shader::createPixel(bakedFragmentShader);
        const auto program = gpu::Shader::createProgram(vs, ps);
        
        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding("diffuseMap", Diffuse));
        slotBindings.insert(gpu::Shader::Binding("normalMap", Normal));
        slotBindings.insert(gpu::Shader::Binding("specularMap", Specular));
        slotBindings.insert(gpu::Shader::Binding("depthMap", Depth));
        slotBindings.insert(gpu::Shader::Binding("lightingMap", Lighting));
        slotBindings.insert(gpu::Shader::Binding("shadowMap", Shadow));
        slotBindings.insert(gpu::Shader::Binding("pyramidMap", Pyramid));
        slotBindings.insert(gpu::Shader::Binding("occlusionMap", AmbientOcclusion));
        slotBindings.insert(gpu::Shader::Binding("occlusionBlurredMap", AmbientOcclusionBlurred));
        gpu::Shader::makeProgram(*program, slotBindings);
        
        auto pipeline = gpu::Pipeline::create(program, std::make_shared<gpu::State>());
        
        // Good to go add the brand new pipeline
        if (mode != CustomMode) {
            _pipelines[mode] = pipeline;
        } else {
            _customPipelines[customFile].pipeline = pipeline;
        }
    }
    
    if (mode != CustomMode) {
        return _pipelines[mode];
    } else {
        return _customPipelines[customFile].pipeline;
    }
}


void DebugDeferredBuffer::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext) {
    assert(renderContext->getArgs());
    assert(renderContext->getArgs()->_viewFrustum);
    RenderArgs* args = renderContext->getArgs();

    // Guard against unspecified modes
    auto mode = renderContext->_deferredDebugMode;
    if (mode > (int)CustomMode) {
        renderContext->_deferredDebugMode = -1;
        return;
    }

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        const auto geometryBuffer = DependencyManager::get<GeometryCache>();
        const auto framebufferCache = DependencyManager::get<FramebufferCache>();
        const auto& lightStage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();
        
        glm::mat4 projMat;
        Transform viewMat;
        args->_viewFrustum->evalProjectionMatrix(projMat);
        args->_viewFrustum->evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // TODO REMOVE: Temporary until UI
        auto first = _customPipelines.begin()->first;
        
        batch.setPipeline(getPipeline(Modes(renderContext->_deferredDebugMode), first));
        
        batch.setResourceTexture(Diffuse, framebufferCache->getDeferredColorTexture());
        batch.setResourceTexture(Normal, framebufferCache->getDeferredNormalTexture());
        batch.setResourceTexture(Specular, framebufferCache->getDeferredSpecularTexture());
        batch.setResourceTexture(Depth, framebufferCache->getPrimaryDepthTexture());
        batch.setResourceTexture(Lighting, framebufferCache->getLightingTexture());
        batch.setResourceTexture(Shadow, lightStage.lights[0]->shadow.framebuffer->getDepthStencilBuffer());
        batch.setResourceTexture(Pyramid, framebufferCache->getDepthPyramidTexture());
        batch.setResourceTexture(AmbientOcclusion, framebufferCache->getOcclusionTexture());
        batch.setResourceTexture(AmbientOcclusionBlurred, framebufferCache->getOcclusionBlurredTexture());

        const glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
        const glm::vec2 bottomLeft(renderContext->_deferredDebugSize.x, renderContext->_deferredDebugSize.y);
        const glm::vec2 topRight(renderContext->_deferredDebugSize.z, renderContext->_deferredDebugSize.w);
        geometryBuffer->renderQuad(batch, bottomLeft, topRight, color);
    });
}
