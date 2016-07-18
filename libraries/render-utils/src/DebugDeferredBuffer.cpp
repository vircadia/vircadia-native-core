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
#include "TextureCache.h"
#include "DeferredLightingEffect.h"

#include "debug_deferred_buffer_vert.h"
#include "debug_deferred_buffer_frag.h"

using namespace render;

void DebugDeferredBufferConfig::setMode(int newMode) {
    if (newMode == mode) {
        return;
    } else if (newMode > DebugDeferredBuffer::CustomMode || newMode < 0) {
        mode = 0;
    } else {
        mode = newMode;
    }
    emit dirty();
}

enum Slot {
    Albedo = 0,
    Normal,
    Specular,
    Depth,
    Lighting,
    Shadow,
    Pyramid,
    Curvature,
    DiffusedCurvature,
    Scattering,
    AmbientOcclusion,
    AmbientOcclusionBlurred
};



static const std::string DEFAULT_ALBEDO_SHADER {
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(pow(frag.albedo, vec3(1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_METALLIC_SHADER {
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(vec3(pow(frag.metallic, 1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_ROUGHNESS_SHADER {
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(vec3(pow(frag.roughness, 1.0 / 2.2)), 1.0);"
    " }"
};
static const std::string DEFAULT_NORMAL_SHADER {
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(vec3(0.5) + (frag.normal * 0.5), 1.0);"
    " }"
};

static const std::string DEFAULT_OCCLUSION_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(vec3(pow(frag.obscurance, 1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_EMISSIVE_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return (frag.mode == FRAG_MODE_SHADED ? vec4(pow(texture(specularMap, uv).rgb, vec3(1.0 / 2.2)), 1.0) : vec4(vec3(0.0), 1.0));"
    " }"
};

static const std::string DEFAULT_UNLIT_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return (frag.mode == FRAG_MODE_UNLIT ? vec4(pow(frag.albedo, vec3(1.0 / 2.2)), 1.0) : vec4(vec3(0.0), 1.0));"
    " }"
};

static const std::string DEFAULT_LIGHTMAP_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return (frag.mode == FRAG_MODE_LIGHTMAPPED ? vec4(pow(texture(specularMap, uv).rgb, vec3(1.0 / 2.2)), 1.0) : vec4(vec3(0.0), 1.0));"
    " }"
};

static const std::string DEFAULT_SCATTERING_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return (frag.mode == FRAG_MODE_SCATTERING ? vec4(vec3(pow(frag.scattering, 1.0 / 2.2)), 1.0) : vec4(vec3(0.0), 1.0));"
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
    " }"
};

static const std::string DEFAULT_LINEAR_DEPTH_2_SHADER{
    "vec4 getFragmentColor() {"
    //  "    return vec4(vec3(1.0 - texture(pyramidMap, uv).x * 0.01), 1.0);"
    "    return vec4(vec3(1.0 - textureLod(pyramidMap, uv, 1).x * 0.01), 1.0);"
    " }"
};

static const std::string DEFAULT_CURVATURE_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(pow(vec3(texture(curvatureMap, uv).a), vec3(1.0 / 2.2)), 1.0);"
   // "    return vec4(pow(vec3(texture(curvatureMap, uv).xyz), vec3(1.0 / 2.2)), 1.0);"
    //"    return vec4(vec3(1.0 - textureLod(pyramidMap, uv, 3).x * 0.01), 1.0);"
    " }"
};

static const std::string DEFAULT_NORMAL_CURVATURE_SHADER{
    "vec4 getFragmentColor() {"
    //"    return vec4(pow(vec3(texture(curvatureMap, uv).a), vec3(1.0 / 2.2)), 1.0);"
     "    return vec4(vec3(texture(curvatureMap, uv).xyz), 1.0);"
    //"    return vec4(vec3(1.0 - textureLod(pyramidMap, uv, 3).x * 0.01), 1.0);"
    " }"
};

static const std::string DEFAULT_DIFFUSED_CURVATURE_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(pow(vec3(texture(diffusedCurvatureMap, uv).a), vec3(1.0 / 2.2)), 1.0);"
    // "    return vec4(pow(vec3(texture(curvatureMap, uv).xyz), vec3(1.0 / 2.2)), 1.0);"
    //"    return vec4(vec3(1.0 - textureLod(pyramidMap, uv, 3).x * 0.01), 1.0);"
    " }"
};

static const std::string DEFAULT_DIFFUSED_NORMAL_CURVATURE_SHADER{
    "vec4 getFragmentColor() {"
    //"    return vec4(pow(vec3(texture(curvatureMap, uv).a), vec3(1.0 / 2.2)), 1.0);"
    "    return vec4(vec3(texture(diffusedCurvatureMap, uv).xyz), 1.0);"
    //"    return vec4(vec3(1.0 - textureLod(pyramidMap, uv, 3).x * 0.01), 1.0);"
    " }"
};

static const std::string DEFAULT_DEBUG_SCATTERING_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(pow(vec3(texture(scatteringMap, uv).xyz), vec3(1.0 / 2.2)), 1.0);"
  //  "    return vec4(vec3(texture(scatteringMap, uv).xyz), 1.0);"
    " }"
};

static const std::string DEFAULT_AMBIENT_OCCLUSION_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(obscuranceMap, uv).x), 1.0);"
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

std::string DebugDeferredBuffer::getShaderSourceCode(Mode mode, std::string customFile) {
    switch (mode) {
        case AlbedoMode:
            return DEFAULT_ALBEDO_SHADER;
        case MetallicMode:
            return DEFAULT_METALLIC_SHADER;
        case RoughnessMode:
            return DEFAULT_ROUGHNESS_SHADER;
        case NormalMode:
            return DEFAULT_NORMAL_SHADER;
        case DepthMode:
            return DEFAULT_DEPTH_SHADER;
        case EmissiveMode:
            return DEFAULT_EMISSIVE_SHADER;
        case UnlitMode:
            return DEFAULT_UNLIT_SHADER;
        case OcclusionMode:
            return DEFAULT_OCCLUSION_SHADER;
        case LightmapMode:
            return DEFAULT_LIGHTMAP_SHADER;
        case ScatteringMode:
            return DEFAULT_SCATTERING_SHADER;
        case LightingMode:
            return DEFAULT_LIGHTING_SHADER;
        case ShadowMode:
            return DEFAULT_SHADOW_SHADER;
        case LinearDepthMode:
            return DEFAULT_PYRAMID_DEPTH_SHADER;
        case LinearDepth2Mode:
            return DEFAULT_LINEAR_DEPTH_2_SHADER;
        case CurvatureMode:
            return DEFAULT_CURVATURE_SHADER;
        case NormalCurvatureMode:
            return DEFAULT_NORMAL_CURVATURE_SHADER;
        case DiffusedCurvatureMode:
            return DEFAULT_DIFFUSED_CURVATURE_SHADER;
        case DiffusedNormalCurvatureMode:
            return DEFAULT_DIFFUSED_NORMAL_CURVATURE_SHADER;
        case ScatteringDebugMode:
            return DEFAULT_DEBUG_SCATTERING_SHADER;
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

bool DebugDeferredBuffer::pipelineNeedsUpdate(Mode mode, std::string customFile) const {
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

const gpu::PipelinePointer& DebugDeferredBuffer::getPipeline(Mode mode, std::string customFile) {
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
        slotBindings.insert(gpu::Shader::Binding("albedoMap", Albedo));
        slotBindings.insert(gpu::Shader::Binding("normalMap", Normal));
        slotBindings.insert(gpu::Shader::Binding("specularMap", Specular));
        slotBindings.insert(gpu::Shader::Binding("depthMap", Depth));
        slotBindings.insert(gpu::Shader::Binding("obscuranceMap", AmbientOcclusion));
        slotBindings.insert(gpu::Shader::Binding("lightingMap", Lighting));
        slotBindings.insert(gpu::Shader::Binding("shadowMap", Shadow));
        slotBindings.insert(gpu::Shader::Binding("pyramidMap", Pyramid));
        slotBindings.insert(gpu::Shader::Binding("curvatureMap", Curvature));
        slotBindings.insert(gpu::Shader::Binding("diffusedCurvatureMap", DiffusedCurvature));
        slotBindings.insert(gpu::Shader::Binding("scatteringMap", Scattering));
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

void DebugDeferredBuffer::configure(const Config& config) {
    _mode = (Mode)config.mode;
    _size = config.size;
}

void DebugDeferredBuffer::run(const SceneContextPointer& sceneContext, const RenderContextPointer& renderContext, const Inputs& inputs) {
    if (_mode == Off) {
        return;
    }

    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    auto& deferredFramebuffer = inputs.get0();
    auto& surfaceGeometryFramebuffer = inputs.get1();
    auto& diffusedCurvatureFramebuffer = inputs.get2();

    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);

        const auto geometryBuffer = DependencyManager::get<GeometryCache>();
        const auto framebufferCache = DependencyManager::get<FramebufferCache>();
        const auto textureCache = DependencyManager::get<TextureCache>();
        const auto& lightStage = DependencyManager::get<DeferredLightingEffect>()->getLightStage();

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat);
        batch.setModelTransform(Transform());

        // TODO REMOVE: Temporary until UI
        auto first = _customPipelines.begin()->first;

        batch.setPipeline(getPipeline(_mode, first));

        batch.setResourceTexture(Albedo, deferredFramebuffer->getDeferredColorTexture());
        batch.setResourceTexture(Normal, deferredFramebuffer->getDeferredNormalTexture());
        batch.setResourceTexture(Specular, deferredFramebuffer->getDeferredSpecularTexture());
        batch.setResourceTexture(Depth, deferredFramebuffer->getPrimaryDepthTexture());
        batch.setResourceTexture(Lighting, deferredFramebuffer->getLightingTexture());
        batch.setResourceTexture(Shadow, lightStage.lights[0]->shadow.framebuffer->getDepthStencilBuffer());
        batch.setResourceTexture(Pyramid, surfaceGeometryFramebuffer->getLinearDepthTexture());
        batch.setResourceTexture(Curvature, surfaceGeometryFramebuffer->getCurvatureTexture());
        batch.setResourceTexture(DiffusedCurvature, diffusedCurvatureFramebuffer->getRenderBuffer(0));
        if (DependencyManager::get<DeferredLightingEffect>()->isAmbientOcclusionEnabled()) {
            batch.setResourceTexture(AmbientOcclusion, framebufferCache->getOcclusionTexture());
        } else {
            // need to assign the white texture if ao is off
            batch.setResourceTexture(AmbientOcclusion, textureCache->getWhiteTexture());
        }
        batch.setResourceTexture(AmbientOcclusionBlurred, framebufferCache->getOcclusionBlurredTexture());

        const glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
        const glm::vec2 bottomLeft(_size.x, _size.y);
        const glm::vec2 topRight(_size.z, _size.w);
        geometryBuffer->renderQuad(batch, bottomLeft, topRight, color);
    });
}
