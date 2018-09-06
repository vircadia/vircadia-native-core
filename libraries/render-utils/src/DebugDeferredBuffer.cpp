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

#include <QtCore/QFile>
#include <QtCore/QDateTime>

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <render/Scene.h>
#include <ViewFrustum.h>
#include <shaders/Shaders.h>
#include "render-utils/ShaderConstants.h"

#include "GeometryCache.h"
#include "TextureCache.h"
#include "DeferredLightingEffect.h"

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

static const std::string DEFAULT_ALBEDO_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(pow(frag.albedo, vec3(1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_METALLIC_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(vec3(pow(frag.metallic, 1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_ROUGHNESS_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(vec3(pow(frag.roughness, 1.0 / 2.2)), 1.0);"
    // "    return vec4(vec3(pow(colorRamp(frag.roughness), vec3(1.0 / 2.2))), 1.0);"
    " }"
};
static const std::string DEFAULT_NORMAL_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return vec4(vec3(0.5) + (frag.normal * 0.5), 1.0);"
    " }"
};

static const std::string DEFAULT_OCCLUSION_SHADER{
    "vec4 getFragmentColor() {"
    //   "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    //   "    return vec4(vec3(pow(frag.obscurance, 1.0 / 2.2)), 1.0);"
    "    return vec4(vec3(pow(texture(specularMap, uv).a, 1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_EMISSIVE_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return (frag.mode == FRAG_MODE_SHADED ? vec4(pow(texture(specularMap, uv).rgb, vec3(1.0 / 2.2)), 1.0) : "
    "vec4(vec3(0.0), 1.0));"
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
    "    return (frag.mode == FRAG_MODE_LIGHTMAPPED ? vec4(pow(texture(specularMap, uv).rgb, vec3(1.0 / 2.2)), 1.0) : "
    "vec4(vec3(0.0), 1.0));"
    " }"
};

static const std::string DEFAULT_SCATTERING_SHADER{
    "vec4 getFragmentColor() {"
    "    DeferredFragment frag = unpackDeferredFragmentNoPosition(uv);"
    "    return (frag.mode == FRAG_MODE_SCATTERING ? vec4(vec3(pow(frag.scattering, 1.0 / 2.2)), 1.0) : vec4(vec3(0.0), 1.0));"
    " }"
};

static const std::string DEFAULT_DEPTH_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(depthMap, uv).x), 1.0);"
    " }"
};

static const std::string DEFAULT_LIGHTING_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(pow(texture(lightingMap, uv).xyz, vec3(1.0 / 2.2)), 1.0);"
    " }"
};

static const std::string DEFAULT_SHADOW_DEPTH_SHADER{
    "vec4 getFragmentColor() {"
    "    for (int i = 255; i >= 0; --i) {"
    "        float depth = i / 255.0;"
    "        if (texture(shadowMaps, vec4(uv, parameters._shadowCascadeIndex, depth)) > 0.5) {"
    "            return vec4(vec3(depth), 1.0);"
    "        }"
    "    }"
    "    return vec4(vec3(0.0), 1.0);"
    " }"
};

static const std::string DEFAULT_SHADOW_CASCADE_SHADER{
    "vec3 cascadeColors[4] = vec3[4]( vec3(0,1,0), vec3(0,0,1), vec3(1,0,0), vec3(1) );"
    "vec4 getFragmentColor() {"
    "    DeferredFrameTransform deferredTransform = getDeferredFrameTransform();"
    "    DeferredFragment frag = unpackDeferredFragment(deferredTransform, uv);"
    "    vec4 viewPosition = vec4(frag.position.xyz, 1.0);"
    "    float viewDepth = -viewPosition.z;"
    "    vec4 worldPosition = getViewInverse() * viewPosition;"
    "    vec4 cascadeShadowCoords[2];"
    "    ivec2 cascadeIndices;"
    "    float cascadeMix = determineShadowCascadesOnPixel(worldPosition, viewDepth, cascadeShadowCoords, cascadeIndices);"
    "    vec3 firstCascadeColor = cascadeColors[cascadeIndices.x];"
    "    vec3 secondCascadeColor = cascadeColors[cascadeIndices.x];"
    "    if (cascadeMix > 0.0 && cascadeIndices.y < getShadowCascadeCount()) {"
    "       secondCascadeColor = cascadeColors[cascadeIndices.y];"
    "    }"
    "    vec3 color = mix(firstCascadeColor, secondCascadeColor, cascadeMix);"
    "    return vec4(mix(vec3(0.0), color, evalShadowFalloff(viewDepth)), 1.0);"
    "}"
};

static const std::string DEFAULT_LINEAR_DEPTH_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(1.0 - texture(debugTexture0, uv).x * 0.01), 1.0);"
    "}"
};

static const std::string DEFAULT_HALF_LINEAR_DEPTH_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(1.0 - texture(debugTexture0, uv).x * 0.01), 1.0);"
    " }"
};

static const std::string DEFAULT_HALF_NORMAL_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(debugTexture0, uv).xyz), 1.0);"
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

static const std::string DEFAULT_CURVATURE_OCCLUSION_SHADER{
    "vec4 getFragmentColor() {"
    "    vec4 midNormalCurvature;"
    "    vec4 lowNormalCurvature;"
    "    unpackMidLowNormalCurvature(uv, midNormalCurvature, lowNormalCurvature);"
    "    float ambientOcclusion = curvatureAO(lowNormalCurvature.a * 20.0f) * 0.5f;"
    "    float ambientOcclusionHF = curvatureAO(midNormalCurvature.a * 8.0f) * 0.5f;"
    "    ambientOcclusion = min(ambientOcclusion, ambientOcclusionHF);"
    "    return vec4(vec3(ambientOcclusion), 1.0);"
    " }"
};

static const std::string DEFAULT_DEBUG_SCATTERING_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(pow(vec3(texture(debugTexture0, uv).xyz), vec3(1.0 / 2.2)), 1.0);"
    //  "    return vec4(vec3(texture(debugTexture0, uv).xyz), 1.0);"
    " }"
};

static const std::string DEFAULT_AMBIENT_OCCLUSION_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(obscuranceMap, uv).x), 1.0);"
    // When drawing color "    return vec4(vec3(texture(debugTexture0, uv).xyz), 1.0);"
    // when drawing normal"    return vec4(normalize(texture(debugTexture0, uv).xyz * 2.0 - vec3(1.0)), 1.0);"
    " }"
};
static const std::string DEFAULT_AMBIENT_OCCLUSION_BLURRED_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec3(texture(debugTexture0, uv).xyz), 1.0);"
    " }"
};

static const std::string DEFAULT_VELOCITY_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(vec2(texture(debugTexture0, uv).xy), 0.0, 1.0);"
    " }"
};

static const std::string DEFAULT_CUSTOM_SHADER{
    "vec4 getFragmentColor() {"
    "    return vec4(1.0, 0.0, 0.0, 1.0);"
    " }"
};

static std::string getFileContent(std::string fileName, std::string defaultContent = std::string()) {
    QFile customFile(QString::fromStdString(fileName));
    if (customFile.open(QIODevice::ReadOnly)) {
        return customFile.readAll().toStdString();
    }
    qWarning() << "DebugDeferredBuffer::getFileContent(): Could not open" << QString::fromStdString(fileName);
    return defaultContent;
}

#include <QStandardPaths>  // TODO REMOVE: Temporary until UI
DebugDeferredBuffer::DebugDeferredBuffer() {
    // TODO REMOVE: Temporary until UI
    static const auto DESKTOP_PATH = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    static const auto CUSTOM_FILE = DESKTOP_PATH.toStdString() + "/custom.slh";
    CustomPipeline pipeline;
    pipeline.info = QFileInfo(QString::fromStdString(CUSTOM_FILE));
    _customPipelines.emplace(CUSTOM_FILE, pipeline);
    _geometryId = DependencyManager::get<GeometryCache>()->allocateID();
}

DebugDeferredBuffer::~DebugDeferredBuffer() {
    auto geometryCache = DependencyManager::get<GeometryCache>();
    if (geometryCache) {
        geometryCache->releaseID(_geometryId);
    }
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
        case ShadowCascade0Mode:
        case ShadowCascade1Mode:
        case ShadowCascade2Mode:
        case ShadowCascade3Mode:
            return DEFAULT_SHADOW_DEPTH_SHADER;
        case ShadowCascadeIndicesMode:
            return DEFAULT_SHADOW_CASCADE_SHADER;
        case LinearDepthMode:
            return DEFAULT_LINEAR_DEPTH_SHADER;
        case HalfLinearDepthMode:
            return DEFAULT_HALF_LINEAR_DEPTH_SHADER;
        case HalfNormalMode:
            return DEFAULT_HALF_NORMAL_SHADER;
        case CurvatureMode:
            return DEFAULT_CURVATURE_SHADER;
        case NormalCurvatureMode:
            return DEFAULT_NORMAL_CURVATURE_SHADER;
        case DiffusedCurvatureMode:
            return DEFAULT_DIFFUSED_CURVATURE_SHADER;
        case DiffusedNormalCurvatureMode:
            return DEFAULT_DIFFUSED_NORMAL_CURVATURE_SHADER;
        case CurvatureOcclusionMode:
            return DEFAULT_CURVATURE_OCCLUSION_SHADER;
        case ScatteringDebugMode:
            return DEFAULT_DEBUG_SCATTERING_SHADER;
        case AmbientOcclusionMode:
            return DEFAULT_AMBIENT_OCCLUSION_SHADER;
        case AmbientOcclusionBlurredMode:
            return DEFAULT_AMBIENT_OCCLUSION_BLURRED_SHADER;
        case VelocityMode:
            return DEFAULT_VELOCITY_SHADER;
        case CustomMode:
            return getFileContent(customFile, DEFAULT_CUSTOM_SHADER);
        default:
            return DEFAULT_ALBEDO_SHADER;
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
        static const auto FRAGMENT_SHADER_SOURCE =
            gpu::Shader::createPixel(shader::render_utils::fragment::debug_deferred_buffer)->getSource();
        static const std::string SOURCE_PLACEHOLDER{ "//SOURCE_PLACEHOLDER" };
        static const auto SOURCE_PLACEHOLDER_INDEX = FRAGMENT_SHADER_SOURCE.getCode().find(SOURCE_PLACEHOLDER);
        Q_ASSERT_X(SOURCE_PLACEHOLDER_INDEX != std::string::npos, Q_FUNC_INFO, "Could not find source placeholder");

        auto bakedFragmentShader = FRAGMENT_SHADER_SOURCE.getCode();
        bakedFragmentShader.replace(SOURCE_PLACEHOLDER_INDEX, SOURCE_PLACEHOLDER.size(), getShaderSourceCode(mode, customFile));

        const auto vs = gpu::Shader::createVertex(shader::render_utils::vertex::debug_deferred_buffer);
        const auto ps = gpu::Shader::createPixel({ bakedFragmentShader, FRAGMENT_SHADER_SOURCE.getReflection() });
        const auto program = gpu::Shader::createProgram(vs, ps);
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
    auto& parameters = _parameters.edit();

    _mode = (Mode)config.mode;
    _size = config.size;
    parameters._shadowCascadeIndex = glm::clamp(_mode - Mode::ShadowCascade0Mode, 0, (int)SHADOW_CASCADE_MAX_COUNT - 1);
}

void DebugDeferredBuffer::run(const RenderContextPointer& renderContext, const Inputs& inputs) {
    if (_mode == Off) {
        return;
    }

    assert(renderContext->args);
    assert(renderContext->args->hasViewFrustum());
    RenderArgs* args = renderContext->args;

    auto& deferredFramebuffer = inputs.get0();
    auto& linearDepthTarget = inputs.get1();
    auto& surfaceGeometryFramebuffer = inputs.get2();
    auto& ambientOcclusionFramebuffer = inputs.get3();
    auto& velocityFramebuffer = inputs.get4();
    auto& frameTransform = inputs.get5();

    gpu::doInBatch("DebugDeferredBuffer::run", args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);

        const auto geometryBuffer = DependencyManager::get<GeometryCache>();
        const auto textureCache = DependencyManager::get<TextureCache>();

        glm::mat4 projMat;
        Transform viewMat;
        args->getViewFrustum().evalProjectionMatrix(projMat);
        args->getViewFrustum().evalViewTransform(viewMat);
        batch.setProjectionTransform(projMat);
        batch.setViewTransform(viewMat, true);
        batch.setModelTransform(Transform());

        using Textures = render_utils::slot::texture::Texture;
        using UBOs = render_utils::slot::buffer::Buffer;

        // TODO REMOVE: Temporary until UI
        auto first = _customPipelines.begin()->first;
        auto pipeline = getPipeline(_mode, first);
        batch.setPipeline(pipeline);

        if (deferredFramebuffer) {
            batch.setResourceTexture(Textures::DeferredColor, deferredFramebuffer->getDeferredColorTexture());
            batch.setResourceTexture(Textures::DeferredNormal, deferredFramebuffer->getDeferredNormalTexture());
            batch.setResourceTexture(Textures::DeferredSpecular, deferredFramebuffer->getDeferredSpecularTexture());
            batch.setResourceTexture(Textures::DeferredDepth, deferredFramebuffer->getPrimaryDepthTexture());
            batch.setResourceTexture(Textures::DeferredLighting, deferredFramebuffer->getLightingTexture());
        }
        if (velocityFramebuffer && _mode == VelocityMode) {
            batch.setResourceTexture(Textures::DebugTexture0, velocityFramebuffer->getVelocityTexture());
        }

        auto lightStage = renderContext->_scene->getStage<LightStage>();
        assert(lightStage);
        assert(lightStage->getNumLights() > 0);
        auto lightAndShadow = lightStage->getCurrentKeyLightAndShadow();
        const auto& globalShadow = lightAndShadow.second;
        if (globalShadow) {
            batch.setResourceTexture(Textures::Shadow, globalShadow->map);
            batch.setUniformBuffer(UBOs::ShadowParams, globalShadow->getBuffer());
            batch.setUniformBuffer(UBOs::DeferredFrameTransform, frameTransform->getFrameTransformBuffer());
            batch.setUniformBuffer(UBOs::DebugDeferredParams, _parameters);
        }

        if (linearDepthTarget) {
            if (_mode == DepthMode) {
                batch.setResourceTexture(Textures::DebugTexture0, linearDepthTarget->getLinearDepthTexture());
            } else if (_mode == HalfLinearDepthMode) {
                batch.setResourceTexture(Textures::DebugTexture0, linearDepthTarget->getHalfLinearDepthTexture());
            } else if (_mode == HalfNormalMode) {
                batch.setResourceTexture(Textures::DebugTexture0, linearDepthTarget->getHalfNormalTexture());
            }
        }
        if (surfaceGeometryFramebuffer) {
            batch.setResourceTexture(Textures::DeferredCurvature, surfaceGeometryFramebuffer->getCurvatureTexture());
            batch.setResourceTexture(Textures::DeferredDiffusedCurvature,
                                     surfaceGeometryFramebuffer->getLowCurvatureTexture());
        }
        if (ambientOcclusionFramebuffer) {
            if (_mode == AmbientOcclusionMode) {
                batch.setResourceTexture(Textures::DebugTexture0, ambientOcclusionFramebuffer->getOcclusionTexture());
            } else if (_mode == AmbientOcclusionBlurredMode) {
                batch.setResourceTexture(Textures::DebugTexture0, ambientOcclusionFramebuffer->getOcclusionBlurredTexture());
            }
        }
        const glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
        const glm::vec2 bottomLeft(_size.x, _size.y);
        const glm::vec2 topRight(_size.z, _size.w);
        geometryBuffer->renderQuad(batch, bottomLeft, topRight, color, _geometryId);

        batch.setResourceTexture(Textures::DeferredColor, nullptr);
        batch.setResourceTexture(Textures::DeferredNormal, nullptr);
        batch.setResourceTexture(Textures::DeferredSpecular, nullptr);
        batch.setResourceTexture(Textures::DeferredDepth, nullptr);
        batch.setResourceTexture(Textures::DeferredCurvature, nullptr);
        batch.setResourceTexture(Textures::DeferredDiffusedCurvature, nullptr);
        batch.setResourceTexture(Textures::DeferredLighting, nullptr);

        for (auto i = 0; i < SHADOW_CASCADE_MAX_COUNT; i++) {
            batch.setResourceTexture(Textures::Shadow + i, nullptr);
        }

        batch.setResourceTexture(Textures::DebugTexture0, nullptr);
    });
}
