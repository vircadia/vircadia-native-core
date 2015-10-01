//
//  DeferredLightingEffect.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 9/11/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DeferredLightingEffect.h"

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <ViewFrustum.h>

#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>

#include "AbstractViewStateInterface.h"
#include "GeometryCache.h"
#include "TextureCache.h"
#include "FramebufferCache.h"


#include "simple_vert.h"
#include "simple_textured_frag.h"
#include "simple_textured_emisive_frag.h"

#include "deferred_light_vert.h"
#include "deferred_light_limited_vert.h"
#include "deferred_light_spot_vert.h"

#include "directional_light_frag.h"
#include "directional_light_shadow_map_frag.h"
#include "directional_light_cascaded_shadow_map_frag.h"

#include "directional_ambient_light_frag.h"
#include "directional_ambient_light_shadow_map_frag.h"
#include "directional_ambient_light_cascaded_shadow_map_frag.h"

#include "directional_skybox_light_frag.h"
#include "directional_skybox_light_shadow_map_frag.h"
#include "directional_skybox_light_cascaded_shadow_map_frag.h"

#include "point_light_frag.h"
#include "spot_light_frag.h"

static const std::string glowIntensityShaderHandle = "glowIntensity";

struct LightLocations {
    int shadowDistances;
    int shadowScale;
    int radius;
    int ambientSphere;
    int lightBufferUnit;
    int atmosphereBufferUnit;
    int texcoordMat;
    int coneParam;
    int deferredTransformBuffer;
};

static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& program, LightLocationsPtr& locations);


gpu::PipelinePointer DeferredLightingEffect::getPipeline(SimpleProgramKey config) {
    auto it = _simplePrograms.find(config);
    if (it != _simplePrograms.end()) {
        return it.value();
    }
    
    auto state = std::make_shared<gpu::State>();
    if (config.isCulled()) {
        state->setCullMode(gpu::State::CULL_BACK);
    } else {
        state->setCullMode(gpu::State::CULL_NONE);
    }
    state->setDepthTest(true, true, gpu::LESS_EQUAL);
    if (config.hasDepthBias()) {
        state->setDepthBias(1.0f);
        state->setDepthBiasSlopeScale(1.0f);
    }
    state->setBlendFunction(false,
                            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    
    gpu::ShaderPointer program = (config.isEmissive()) ? _emissiveShader : _simpleShader;
    gpu::PipelinePointer pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    _simplePrograms.insert(config, pipeline);
    return pipeline;
}

void DeferredLightingEffect::init(AbstractViewStateInterface* viewState) {
    auto VS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(simple_vert)));
    auto PS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(simple_textured_frag)));
    auto PSEmissive = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(simple_textured_emisive_frag)));
    
    _simpleShader = gpu::ShaderPointer(gpu::Shader::createProgram(VS, PS));
    _emissiveShader = gpu::ShaderPointer(gpu::Shader::createProgram(VS, PSEmissive));
    
    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("normalFittingMap"), DeferredLightingEffect::NORMAL_FITTING_MAP_SLOT));
    gpu::Shader::makeProgram(*_simpleShader, slotBindings);
    gpu::Shader::makeProgram(*_emissiveShader, slotBindings);

    _viewState = viewState;
    _directionalLightLocations = std::make_shared<LightLocations>();
    _directionalLightShadowMapLocations = std::make_shared<LightLocations>();
    _directionalLightCascadedShadowMapLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightShadowMapLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightCascadedShadowMapLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightShadowMapLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightCascadedShadowMapLocations = std::make_shared<LightLocations>();
    _pointLightLocations = std::make_shared<LightLocations>();
    _spotLightLocations = std::make_shared<LightLocations>();

    loadLightProgram(deferred_light_vert, directional_light_frag, false, _directionalLight, _directionalLightLocations);
    loadLightProgram(deferred_light_vert, directional_light_shadow_map_frag, false, _directionalLightShadowMap,
        _directionalLightShadowMapLocations);
    loadLightProgram(deferred_light_vert, directional_light_cascaded_shadow_map_frag, false, _directionalLightCascadedShadowMap,
        _directionalLightCascadedShadowMapLocations);

    loadLightProgram(deferred_light_vert, directional_ambient_light_frag, false, _directionalAmbientSphereLight, _directionalAmbientSphereLightLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_shadow_map_frag, false, _directionalAmbientSphereLightShadowMap,
        _directionalAmbientSphereLightShadowMapLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_cascaded_shadow_map_frag, false, _directionalAmbientSphereLightCascadedShadowMap,
        _directionalAmbientSphereLightCascadedShadowMapLocations);

    loadLightProgram(deferred_light_vert, directional_skybox_light_frag, false, _directionalSkyboxLight, _directionalSkyboxLightLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_shadow_map_frag, false, _directionalSkyboxLightShadowMap,
        _directionalSkyboxLightShadowMapLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_cascaded_shadow_map_frag, false, _directionalSkyboxLightCascadedShadowMap,
        _directionalSkyboxLightCascadedShadowMapLocations);


    loadLightProgram(deferred_light_limited_vert, point_light_frag, true, _pointLight, _pointLightLocations);
    loadLightProgram(deferred_light_spot_vert, spot_light_frag, true, _spotLight, _spotLightLocations);

    {
        //auto VSFS = gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS();
        //auto PSBlit = gpu::StandardShaderLib::getDrawTexturePS();
        auto blitProgram = gpu::StandardShaderLib::getProgram(gpu::StandardShaderLib::getDrawViewportQuadTransformTexcoordVS, gpu::StandardShaderLib::getDrawTexturePS);
        gpu::Shader::makeProgram(*blitProgram);
        auto blitState = std::make_shared<gpu::State>();
        blitState->setBlendFunction(true,
                                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
        blitState->setColorWriteMask(true, true, true, false);
        _blitLightBuffer = gpu::PipelinePointer(gpu::Pipeline::create(blitProgram, blitState));
    }

    // Allocate a global light representing the Global Directional light casting shadow (the sun) and the ambient light
    _globalLights.push_back(0);
    _allocatedLights.push_back(std::make_shared<model::Light>());

    model::LightPointer lp = _allocatedLights[0];

    lp->setDirection(-glm::vec3(1.0f, 1.0f, 1.0f));
    lp->setColor(glm::vec3(1.0f));
    lp->setIntensity(1.0f);
    lp->setType(model::Light::SUN);
    lp->setAmbientSpherePreset(gpu::SphericalHarmonics::Preset(_ambientLightMode % gpu::SphericalHarmonics::NUM_PRESET));
}



gpu::PipelinePointer DeferredLightingEffect::bindSimpleProgram(gpu::Batch& batch, bool textured, bool culled,
                                               bool emmisive, bool depthBias) {
    SimpleProgramKey config{textured, culled, emmisive, depthBias};
    gpu::PipelinePointer pipeline = getPipeline(config);
    batch.setPipeline(pipeline);

    gpu::ShaderPointer program = (config.isEmissive()) ? _emissiveShader : _simpleShader;
    int glowIntensity = program->getUniforms().findLocation("glowIntensity");
    batch._glUniform1f(glowIntensity, 1.0f);
    
    if (!config.isTextured()) {
        // If it is not textured, bind white texture and keep using textured pipeline
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }

    batch.setResourceTexture(NORMAL_FITTING_MAP_SLOT, DependencyManager::get<TextureCache>()->getNormalFittingTexture());
    return pipeline;
}

uint32_t toCompactColor(const glm::vec4& color) {
    uint32_t compactColor = ((int(color.x * 255.0f) & 0xFF)) |
        ((int(color.y * 255.0f) & 0xFF) << 8) |
        ((int(color.z * 255.0f) & 0xFF) << 16) |
        ((int(color.w * 255.0f) & 0xFF) << 24);
    return compactColor;
}

static const size_t INSTANCE_TRANSFORM_BUFFER = 0;
static const size_t INSTANCE_COLOR_BUFFER = 1;

template <typename F>
void renderInstances(const std::string& name, gpu::Batch& batch, const Transform& transform, const glm::vec4& color, F f) {
    {
        gpu::BufferPointer instanceTransformBuffer = batch.getNamedBuffer(name, INSTANCE_TRANSFORM_BUFFER);
        glm::mat4 glmTransform;
        instanceTransformBuffer->append(transform.getMatrix(glmTransform));

        gpu::BufferPointer instanceColorBuffer = batch.getNamedBuffer(name, INSTANCE_COLOR_BUFFER);
        auto compactColor = toCompactColor(color);
        instanceColorBuffer->append(compactColor);
    }

    auto that = DependencyManager::get<DeferredLightingEffect>();
    batch.setupNamedCalls(name, [=](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
        auto pipeline = that->bindSimpleProgram(batch);
        auto location = pipeline->getProgram()->getUniforms().findLocation("Instanced");

        batch._glUniform1i(location, 1);
        f(batch, data);
        batch._glUniform1i(location, 0);
    });
}

void DeferredLightingEffect::renderSolidSphereInstance(gpu::Batch& batch, const Transform& transform, const glm::vec4& color) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    renderInstances(INSTANCE_NAME, batch, transform, color, [](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
        DependencyManager::get<GeometryCache>()->renderShapeInstances(batch, GeometryCache::Sphere, data._count,
            data._buffers[INSTANCE_TRANSFORM_BUFFER], data._buffers[INSTANCE_COLOR_BUFFER]);
    });
}

void DeferredLightingEffect::renderWireSphereInstance(gpu::Batch& batch, const Transform& transform, const glm::vec4& color) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    renderInstances(INSTANCE_NAME, batch, transform, color, [](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
        DependencyManager::get<GeometryCache>()->renderWireShapeInstances(batch, GeometryCache::Sphere, data._count,
            data._buffers[INSTANCE_TRANSFORM_BUFFER], data._buffers[INSTANCE_COLOR_BUFFER]);
    });
}

// Enable this in a debug build to cause 'box' entities to iterate through all the 
// available shape types, both solid and wireframes
//#define DEBUG_SHAPES

void DeferredLightingEffect::renderSolidCubeInstance(gpu::Batch& batch, const Transform& transform, const glm::vec4& color) {
    static const std::string INSTANCE_NAME = __FUNCTION__;

#ifdef DEBUG_SHAPES
    static auto startTime = usecTimestampNow();
    renderInstances(INSTANCE_NAME, batch, transform, color, [](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {

        auto usecs = usecTimestampNow();
        usecs -= startTime;
        auto msecs = usecs / USECS_PER_MSEC;
        float seconds = msecs;
        seconds /= MSECS_PER_SECOND;
        float fractionalSeconds = seconds - floor(seconds);
        int shapeIndex = (int)seconds;

        // Every second we flip to the next shape.
        static const int SHAPE_COUNT = 5;
        GeometryCache::Shape shapes[SHAPE_COUNT] = {
            GeometryCache::Cube,
            GeometryCache::Tetrahedron,
            GeometryCache::Sphere,
            GeometryCache::Icosahedron,
            GeometryCache::Line,
        };

        shapeIndex %= SHAPE_COUNT;
        GeometryCache::Shape shape = shapes[shapeIndex];

        // For the first half second for a given shape, show the wireframe, for the second half, show the solid.
        if (fractionalSeconds > 0.5f) {
            DependencyManager::get<GeometryCache>()->renderShapeInstances(batch, shape, data._count,
                data._buffers[INSTANCE_TRANSFORM_BUFFER], data._buffers[INSTANCE_COLOR_BUFFER]);
        } else {
            DependencyManager::get<GeometryCache>()->renderWireShapeInstances(batch, shape, data._count,
                data._buffers[INSTANCE_TRANSFORM_BUFFER], data._buffers[INSTANCE_COLOR_BUFFER]);
        }
    });
#else
    renderInstances(INSTANCE_NAME, batch, transform, color, [](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
        DependencyManager::get<GeometryCache>()->renderCubeInstances(batch, data._count,
            data._buffers[INSTANCE_TRANSFORM_BUFFER], data._buffers[INSTANCE_COLOR_BUFFER]);
    });
#endif
}

void DeferredLightingEffect::renderWireCubeInstance(gpu::Batch& batch, const Transform& transform, const glm::vec4& color) {
    static const std::string INSTANCE_NAME = __FUNCTION__;
    renderInstances(INSTANCE_NAME, batch, transform, color, [](gpu::Batch& batch, gpu::Batch::NamedBatchData& data) {
        DependencyManager::get<GeometryCache>()->renderWireCubeInstances(batch, data._count,
            data._buffers[INSTANCE_TRANSFORM_BUFFER], data._buffers[INSTANCE_COLOR_BUFFER]);
    });
}

void DeferredLightingEffect::renderQuad(gpu::Batch& batch, const glm::vec3& minCorner, const glm::vec3& maxCorner,
                                        const glm::vec4& color) {
    bindSimpleProgram(batch);
    DependencyManager::get<GeometryCache>()->renderQuad(batch, minCorner, maxCorner, color);
}

void DeferredLightingEffect::renderLine(gpu::Batch& batch, const glm::vec3& p1, const glm::vec3& p2,
                                        const glm::vec4& color1, const glm::vec4& color2) {
    bindSimpleProgram(batch);
    DependencyManager::get<GeometryCache>()->renderLine(batch, p1, p2, color1, color2);
}

void DeferredLightingEffect::addPointLight(const glm::vec3& position, float radius, const glm::vec3& color,
        float intensity) {
    addSpotLight(position, radius, color, intensity);    
}

void DeferredLightingEffect::addSpotLight(const glm::vec3& position, float radius, const glm::vec3& color,
    float intensity, const glm::quat& orientation, float exponent, float cutoff) {
    
    unsigned int lightID = _pointLights.size() + _spotLights.size() + _globalLights.size();
    if (lightID >= _allocatedLights.size()) {
        _allocatedLights.push_back(std::make_shared<model::Light>());
    }
    model::LightPointer lp = _allocatedLights[lightID];

    lp->setPosition(position);
    lp->setMaximumRadius(radius);
    lp->setColor(color);
    lp->setIntensity(intensity);
    //lp->setShowContour(quadraticAttenuation);

    if (exponent == 0.0f && cutoff == PI) {
        lp->setType(model::Light::POINT);
        _pointLights.push_back(lightID);
        
    } else {
        lp->setOrientation(orientation);
        lp->setSpotAngle(cutoff);
        lp->setSpotExponent(exponent);
        lp->setType(model::Light::SPOT);
        _spotLights.push_back(lightID);
    }
}

void DeferredLightingEffect::prepare(RenderArgs* args) {
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setStateScissorRect(args->_viewport);

        auto primaryFbo = DependencyManager::get<FramebufferCache>()->getPrimaryFramebuffer();

        batch.setFramebuffer(primaryFbo);
        // clear the normal and specular buffers
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), true);
        const float MAX_SPECULAR_EXPONENT = 128.0f;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR2, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f / MAX_SPECULAR_EXPONENT), true);
    });
}

gpu::FramebufferPointer _copyFBO;

void DeferredLightingEffect::render(RenderArgs* args) {
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        
        // Allocate the parameters buffer used by all the deferred shaders
        if (!_deferredTransformBuffer[0]._buffer) {
            DeferredTransform parameters;
            _deferredTransformBuffer[0] = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(DeferredTransform), (const gpu::Byte*) &parameters));
            _deferredTransformBuffer[1] = gpu::BufferView(std::make_shared<gpu::Buffer>(sizeof(DeferredTransform), (const gpu::Byte*) &parameters));
        }

        // Framebuffer copy operations cannot function as multipass stereo operations.  
        batch.enableStereo(false);

        // perform deferred lighting, rendering to free fbo
        auto framebufferCache = DependencyManager::get<FramebufferCache>();
    
        QSize framebufferSize = framebufferCache->getFrameBufferSize();
    
        // binding the first framebuffer
        _copyFBO = framebufferCache->getFramebuffer();
        batch.setFramebuffer(_copyFBO);

        // Clearing it
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);
        batch.clearColorFramebuffer(_copyFBO->getBufferMask(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), true);

        // BInd the G-Buffer surfaces
        batch.setResourceTexture(0, framebufferCache->getPrimaryColorTexture());
        batch.setResourceTexture(1, framebufferCache->getPrimaryNormalTexture());
        batch.setResourceTexture(2, framebufferCache->getPrimarySpecularTexture());
        batch.setResourceTexture(3, framebufferCache->getPrimaryDepthTexture());

        // THe main viewport is assumed to be the mono viewport (or the 2 stereo faces side by side within that viewport)
        auto monoViewport = args->_viewport;
        float sMin = args->_viewport.x / (float)framebufferSize.width();
        float sWidth = args->_viewport.z / (float)framebufferSize.width();
        float tMin = args->_viewport.y / (float)framebufferSize.height();
        float tHeight = args->_viewport.w / (float)framebufferSize.height();

        // The view frustum is the mono frustum base
        auto viewFrustum = args->_viewFrustum;

        // Eval the mono projection
        mat4 monoProjMat;
        viewFrustum->evalProjectionMatrix(monoProjMat);

        // The mono view transform
        Transform monoViewTransform;
        viewFrustum->evalViewTransform(monoViewTransform);

        // THe mono view matrix coming from the mono view transform
        glm::mat4 monoViewMat;
        monoViewTransform.getMatrix(monoViewMat);

        // Running in stero ?
        bool isStereo = args->_context->isStereo();
        int numPasses = 1;

        mat4 projMats[2];
        Transform viewTransforms[2];
        ivec4 viewports[2];
        vec4 clipQuad[2];
        vec2 screenBottomLeftCorners[2];
        vec2 screenTopRightCorners[2];
        vec4 fetchTexcoordRects[2];

        DeferredTransform deferredTransforms[2];
        auto geometryCache = DependencyManager::get<GeometryCache>();

        if (isStereo) {
            numPasses = 2;

            mat4 eyeViews[2];
            args->_context->getStereoProjections(projMats);
            args->_context->getStereoViews(eyeViews);

            float halfWidth = 0.5f * sWidth;

            for (int i = 0; i < numPasses; i++) {
                // In stereo, the 2 sides are layout side by side in the mono viewport and their width is half
                int sideWidth = monoViewport.z >> 1;
                viewports[i] = ivec4(monoViewport.x + (i * sideWidth), monoViewport.y, sideWidth, monoViewport.w);

                deferredTransforms[i].projection = projMats[i];

                auto sideViewMat = monoViewMat * glm::inverse(eyeViews[i]);
                viewTransforms[i].evalFromRawMatrix(sideViewMat);
                deferredTransforms[i].viewInverse = sideViewMat;

                deferredTransforms[i].stereoSide = (i == 0 ? -1.0f : 1.0f);

                clipQuad[i] = glm::vec4(sMin + i * halfWidth, tMin, halfWidth, tHeight);
                screenBottomLeftCorners[i] = glm::vec2(-1.0f + i * 1.0f, -1.0f);
                screenTopRightCorners[i] = glm::vec2(i * 1.0f, 1.0f);

                fetchTexcoordRects[i] = glm::vec4(sMin + i * halfWidth, tMin, halfWidth, tHeight);
            }
        } else {

            viewports[0] = monoViewport;
            projMats[0] = monoProjMat;

            deferredTransforms[0].projection = monoProjMat;
     
            deferredTransforms[0].viewInverse = monoViewMat;
            viewTransforms[0] = monoViewTransform;

            deferredTransforms[0].stereoSide = 0.0f;

            clipQuad[0] = glm::vec4(sMin, tMin, sWidth, tHeight);
            screenBottomLeftCorners[0] = glm::vec2(-1.0f, -1.0f);
            screenTopRightCorners[0] = glm::vec2(1.0f, 1.0f);

            fetchTexcoordRects[0] = glm::vec4(sMin, tMin, sWidth, tHeight);
        }

        auto eyePoint = viewFrustum->getPosition();
        float nearRadius = glm::distance(eyePoint, viewFrustum->getNearTopLeft());


        for (int side = 0; side < numPasses; side++) {
            // Render in this side's viewport
            batch.setViewportTransform(viewports[side]);
            batch.setStateScissorRect(viewports[side]);

            // Sync and Bind the correct DeferredTransform ubo
            _deferredTransformBuffer[side]._buffer->setSubData(0, sizeof(DeferredTransform), (const gpu::Byte*) &deferredTransforms[side]);
            batch.setUniformBuffer(_directionalLightLocations->deferredTransformBuffer, _deferredTransformBuffer[side]);

            glm::vec2 topLeft(-1.0f, -1.0f);
            glm::vec2 bottomRight(1.0f, 1.0f);
            glm::vec2 texCoordTopLeft(clipQuad[side].x, clipQuad[side].y);
            glm::vec2 texCoordBottomRight(clipQuad[side].x + clipQuad[side].z, clipQuad[side].y + clipQuad[side].w);

            // First Global directional light and ambient pass
            {
                bool useSkyboxCubemap = (_skybox) && (_skybox->getCubemap());

                auto& program = _directionalLight;
                LightLocationsPtr locations = _directionalLightLocations;

                // TODO: At some point bring back the shadows...
                // Setup the global directional pass pipeline
                {
                    if (useSkyboxCubemap) {
                        program = _directionalSkyboxLight;
                        locations = _directionalSkyboxLightLocations;
                    } else if (_ambientLightMode > -1) {
                        program = _directionalAmbientSphereLight;
                        locations = _directionalAmbientSphereLightLocations;
                    }
                    batch.setPipeline(program);
                }

                { // Setup the global lighting
                    auto globalLight = _allocatedLights[_globalLights.front()];
    
                    if (locations->ambientSphere >= 0) {
                        gpu::SphericalHarmonics sh = globalLight->getAmbientSphere();
                        if (useSkyboxCubemap && _skybox->getCubemap()->getIrradiance()) {
                            sh = (*_skybox->getCubemap()->getIrradiance());
                        }
                        for (int i =0; i <gpu::SphericalHarmonics::NUM_COEFFICIENTS; i++) {
                           batch._glUniform4fv(locations->ambientSphere + i, 1, (const float*) (&sh) + i * 4);
                        }
                    }
    
                    if (useSkyboxCubemap) {
                        batch.setResourceTexture(5, _skybox->getCubemap());
                    }

                    if (locations->lightBufferUnit >= 0) {
                        batch.setUniformBuffer(locations->lightBufferUnit, globalLight->getSchemaBuffer());
                    }
        
                    if (_atmosphere && (locations->atmosphereBufferUnit >= 0)) {
                        batch.setUniformBuffer(locations->atmosphereBufferUnit, _atmosphere->getDataBuffer());
                    }
                }

                {
                    batch.setModelTransform(Transform());
                    batch.setProjectionTransform(glm::mat4());
                    batch.setViewTransform(Transform());

                    glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
                   geometryCache->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
                }

                if (useSkyboxCubemap) {
                    batch.setResourceTexture(5, nullptr);
                }
            }

            auto texcoordMat = glm::mat4();
          /*  texcoordMat[0] = glm::vec4(sWidth / 2.0f, 0.0f, 0.0f, sMin + sWidth / 2.0f);
            texcoordMat[1] = glm::vec4(0.0f, tHeight / 2.0f, 0.0f, tMin + tHeight / 2.0f);
           */ texcoordMat[0] = glm::vec4(fetchTexcoordRects[side].z / 2.0f, 0.0f, 0.0f, fetchTexcoordRects[side].x + fetchTexcoordRects[side].z / 2.0f);
            texcoordMat[1] = glm::vec4(0.0f, fetchTexcoordRects[side].w / 2.0f, 0.0f, fetchTexcoordRects[side].y + fetchTexcoordRects[side].w / 2.0f);
            texcoordMat[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
            texcoordMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            // enlarge the scales slightly to account for tesselation
            const float SCALE_EXPANSION = 0.05f;


            batch.setProjectionTransform(projMats[side]);
            batch.setViewTransform(viewTransforms[side]);

            // Splat Point lights
            if (!_pointLights.empty()) {
                batch.setPipeline(_pointLight);

                batch._glUniformMatrix4fv(_pointLightLocations->texcoordMat, 1, false, reinterpret_cast< const float* >(&texcoordMat));

                for (auto lightID : _pointLights) {
                    auto& light = _allocatedLights[lightID];
                    // IN DEBUG: light->setShowContour(true);
                    batch.setUniformBuffer(_pointLightLocations->lightBufferUnit, light->getSchemaBuffer());

                    float expandedRadius = light->getMaximumRadius() * (1.0f + SCALE_EXPANSION);
                    // TODO: We shouldn;t have to do that test and use a different volume geometry for when inside the vlight volume,
                    // we should be able to draw thre same geometry use DepthClamp but for unknown reason it's s not working...
                    if (glm::distance(eyePoint, glm::vec3(light->getPosition())) < expandedRadius + nearRadius) {
                        Transform model;
                        model.setTranslation(glm::vec3(0.0f, 0.0f, -1.0f));
                        batch.setModelTransform(model);
                        batch.setViewTransform(Transform());
                        batch.setProjectionTransform(glm::mat4());

                        glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
                        DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
                
                        batch.setProjectionTransform(projMats[side]);
                        batch.setViewTransform(viewTransforms[side]);
                    } else {
                        Transform model;
                        model.setTranslation(glm::vec3(light->getPosition().x, light->getPosition().y, light->getPosition().z));
                        batch.setModelTransform(model.postScale(expandedRadius));
                        batch._glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                        geometryCache->renderSphere(batch);
                    }
                }
            }
    
            // Splat spot lights
            if (!_spotLights.empty()) {
                batch.setPipeline(_spotLight);

                batch._glUniformMatrix4fv(_spotLightLocations->texcoordMat, 1, false, reinterpret_cast< const float* >(&texcoordMat));

                for (auto lightID : _spotLights) {
                    auto light = _allocatedLights[lightID];
                    // IN DEBUG: light->setShowContour(true);
                    batch.setUniformBuffer(_spotLightLocations->lightBufferUnit, light->getSchemaBuffer());

                    auto eyeLightPos = eyePoint - light->getPosition();
                    auto eyeHalfPlaneDistance = glm::dot(eyeLightPos, light->getDirection());

                    const float TANGENT_LENGTH_SCALE = 0.666f;
                    glm::vec4 coneParam(light->getSpotAngleCosSin(), TANGENT_LENGTH_SCALE * tanf(0.5f * light->getSpotAngle()), 1.0f);

                    float expandedRadius = light->getMaximumRadius() * (1.0f + SCALE_EXPANSION);
                    // TODO: We shouldn;t have to do that test and use a different volume geometry for when inside the vlight volume,
                    // we should be able to draw thre same geometry use DepthClamp but for unknown reason it's s not working...
                    if ((eyeHalfPlaneDistance > -nearRadius) &&
                        (glm::distance(eyePoint, glm::vec3(light->getPosition())) < expandedRadius + nearRadius)) {
                        coneParam.w = 0.0f;
                        batch._glUniform4fv(_spotLightLocations->coneParam, 1, reinterpret_cast< const float* >(&coneParam));

                        Transform model;
                        model.setTranslation(glm::vec3(0.0f, 0.0f, -1.0f));
                        batch.setModelTransform(model);
                        batch.setViewTransform(Transform());
                        batch.setProjectionTransform(glm::mat4());
                
                        glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
                        DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
                
                        batch.setProjectionTransform( projMats[side]);
                        batch.setViewTransform(viewTransforms[side]);
                    } else {
                        coneParam.w = 1.0f;
                        batch._glUniform4fv(_spotLightLocations->coneParam, 1, reinterpret_cast< const float* >(&coneParam));

                        Transform model;
                        model.setTranslation(light->getPosition());
                        model.postRotate(light->getOrientation());
                        model.postScale(glm::vec3(expandedRadius, expandedRadius, expandedRadius));

                        batch.setModelTransform(model);
                        auto mesh = getSpotLightMesh();

                        batch.setIndexBuffer(mesh->getIndexBuffer());
                        batch.setInputBuffer(0, mesh->getVertexBuffer());
                        batch.setInputFormat(mesh->getVertexFormat());

                        auto& part = mesh->getPartBuffer().get<model::Mesh::Part>();

                        batch.drawIndexed(model::Mesh::topologyToPrimitive(part._topology), part._numIndices, part._startIndex);
                    }
                }
            }
        }

        // Probably not necessary in the long run because the gpu layer would unbound this texture if used as render target
        batch.setResourceTexture(0, nullptr);
        batch.setResourceTexture(1, nullptr);
        batch.setResourceTexture(2, nullptr);
        batch.setResourceTexture(3, nullptr);
        batch.setUniformBuffer(_directionalLightLocations->deferredTransformBuffer, nullptr);
    });

    // End of the Lighting pass
    if (!_pointLights.empty()) {
        _pointLights.clear();
    }
    if (!_spotLights.empty()) {
        _spotLights.clear();
    }
}


void DeferredLightingEffect::copyBack(RenderArgs* args) {
    auto framebufferCache = DependencyManager::get<FramebufferCache>();
    gpu::doInBatch(args->_context, [=](gpu::Batch& batch) {
        batch.enableStereo(false);
        QSize framebufferSize = framebufferCache->getFrameBufferSize();

        // TODO why doesn't this blit work?  It only seems to affect a small area below the rear view mirror.
        //  auto destFbo = framebufferCache->getPrimaryFramebuffer();
        auto destFbo = framebufferCache->getPrimaryFramebufferDepthColor();
        //    gpu::Vec4i vp = args->_viewport;
        //    batch.blit(_copyFBO, vp, framebufferCache->getPrimaryFramebuffer(), vp);
        batch.setFramebuffer(destFbo);
        batch.setViewportTransform(args->_viewport);
        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());
        {
            float sMin = args->_viewport.x / (float)framebufferSize.width();
            float sWidth = args->_viewport.z / (float)framebufferSize.width();
            float tMin = args->_viewport.y / (float)framebufferSize.height();
            float tHeight = args->_viewport.w / (float)framebufferSize.height();
            Transform model;
            batch.setPipeline(_blitLightBuffer);
            model.setTranslation(glm::vec3(sMin, tMin, 0.0));
            model.setScale(glm::vec3(sWidth, tHeight, 1.0));
            batch.setModelTransform(model);
        }

        batch.setResourceTexture(0, _copyFBO->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);

        args->_context->render(batch);
    });
    framebufferCache->releaseFramebuffer(_copyFBO);
}

void DeferredLightingEffect::setupTransparent(RenderArgs* args, int lightBufferUnit) {
    auto globalLight = _allocatedLights[_globalLights.front()];
    args->_batch->setUniformBuffer(lightBufferUnit, globalLight->getSchemaBuffer());
}

static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& pipeline, LightLocationsPtr& locations) {
    auto VS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(vertSource)));
    auto PS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(fragSource)));
    
    gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(VS, PS));

    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("diffuseMap"), 0));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), 1));
    slotBindings.insert(gpu::Shader::Binding(std::string("specularMap"), 2));
    slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), 3));
    slotBindings.insert(gpu::Shader::Binding(std::string("shadowMap"), 4));
    slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), 5));
    const int LIGHT_GPU_SLOT = 3;
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), LIGHT_GPU_SLOT));
    const int ATMOSPHERE_GPU_SLOT = 4;
    slotBindings.insert(gpu::Shader::Binding(std::string("atmosphereBufferUnit"), ATMOSPHERE_GPU_SLOT));

    slotBindings.insert(gpu::Shader::Binding(std::string("deferredTransformBuffer"), DeferredLightingEffect::DEFERRED_TRANSFORM_BUFFER_SLOT));

    gpu::Shader::makeProgram(*program, slotBindings);

    locations->shadowDistances = program->getUniforms().findLocation("shadowDistances");
    locations->shadowScale = program->getUniforms().findLocation("shadowScale");

    locations->radius = program->getUniforms().findLocation("radius");
    locations->ambientSphere = program->getUniforms().findLocation("ambientSphere.L00");

    locations->texcoordMat = program->getUniforms().findLocation("texcoordMat");
    locations->coneParam = program->getUniforms().findLocation("coneParam");

    locations->lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
    locations->atmosphereBufferUnit = program->getBuffers().findLocation("atmosphereBufferUnit");
    locations->deferredTransformBuffer = program->getBuffers().findLocation("deferredTransformBuffer");

    auto state = std::make_shared<gpu::State>();
    if (lightVolume) {
        state->setCullMode(gpu::State::CULL_BACK);
        
        // No need for z test since the depth buffer is not bound state->setDepthTest(true, false, gpu::LESS_EQUAL);
        // TODO: We should bind the true depth buffer both as RT and texture for the depth test
        // TODO: We should use DepthClamp and avoid changing geometry for inside /outside cases
        state->setDepthClampEnable(true);

        // additive blending
        state->setBlendFunction(true, gpu::State::ONE, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    } else {
        state->setCullMode(gpu::State::CULL_BACK);
    }
    pipeline.reset(gpu::Pipeline::create(program, state));

}

void DeferredLightingEffect::setAmbientLightMode(int preset) {
    if ((preset >= 0) && (preset < gpu::SphericalHarmonics::NUM_PRESET)) {
        _ambientLightMode = preset;
        auto light = _allocatedLights.front();
        light->setAmbientSpherePreset(gpu::SphericalHarmonics::Preset(preset % gpu::SphericalHarmonics::NUM_PRESET));
    } else {
        // force to preset 0
        setAmbientLightMode(0);
    }
}

void DeferredLightingEffect::setGlobalLight(const glm::vec3& direction, const glm::vec3& diffuse, float intensity, float ambientIntensity) {
    auto light = _allocatedLights.front();
    light->setDirection(direction);
    light->setColor(diffuse);
    light->setIntensity(intensity);
    light->setAmbientIntensity(ambientIntensity);
}

void DeferredLightingEffect::setGlobalSkybox(const model::SkyboxPointer& skybox) {
    _skybox = skybox;
}

model::MeshPointer DeferredLightingEffect::getSpotLightMesh() {
    if (!_spotLightMesh) {
        _spotLightMesh = std::make_shared<model::Mesh>();

        int slices = 32;
        int rings = 3;
        int vertices = 2 + rings * slices;
        int originVertex = vertices - 2;
        int capVertex = vertices - 1;
        int verticesSize = vertices * 3 * sizeof(float);
        int indices = 3 * slices * (1 + 1 + 2 * (rings -1));
        int ringFloatOffset = slices * 3;


        float* vertexData = new float[verticesSize];
        float* vertexRing0 = vertexData;
        float* vertexRing1 = vertexRing0 + ringFloatOffset;
        float* vertexRing2 = vertexRing1 + ringFloatOffset;
        
        for (int i = 0; i < slices; i++) {
            float theta = TWO_PI * i / slices;
            auto cosin = glm::vec2(cosf(theta), sinf(theta));

            *(vertexRing0++) = cosin.x;
            *(vertexRing0++) = cosin.y;
            *(vertexRing0++) = 0.0f;

            *(vertexRing1++) = cosin.x;
            *(vertexRing1++) = cosin.y;
            *(vertexRing1++) = 0.33f;

            *(vertexRing2++) = cosin.x;
            *(vertexRing2++) = cosin.y;
            *(vertexRing2++) = 0.66f;
        }
        
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = -1.0f;
        
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = 0.0f;
        *(vertexRing2++) = 1.0f;
        
        _spotLightMesh->setVertexBuffer(gpu::BufferView(new gpu::Buffer(verticesSize, (gpu::Byte*) vertexData), gpu::Element::VEC3F_XYZ));
        delete[] vertexData;

        gpu::uint16* indexData = new gpu::uint16[indices];
        gpu::uint16* index = indexData;
        for (int i = 0; i < slices; i++) {
            *(index++) = originVertex;
            
            int s0 = i;
            int s1 = ((i + 1) % slices);
            *(index++) = s0;
            *(index++) = s1;

            int s2 = s0 + slices;
            int s3 = s1 + slices;
            *(index++) = s1;
            *(index++) = s0;
            *(index++) = s2;

            *(index++) = s1;
            *(index++) = s2;
            *(index++) = s3;

            int s4 = s2 + slices;
            int s5 = s3 + slices;
            *(index++) = s3;
            *(index++) = s2;
            *(index++) = s4;

            *(index++) = s3;
            *(index++) = s4;
            *(index++) = s5;


            *(index++) = s5;
            *(index++) = s4;
            *(index++) = capVertex;
        }

        _spotLightMesh->setIndexBuffer(gpu::BufferView(new gpu::Buffer(sizeof(unsigned short) * indices, (gpu::Byte*) indexData), gpu::Element::INDEX_UINT16));
        delete[] indexData;

        model::Mesh::Part part(0, indices, 0, model::Mesh::TRIANGLES);
        //DEBUG: model::Mesh::Part part(0, indices, 0, model::Mesh::LINE_STRIP);
        
        _spotLightMesh->setPartBuffer(gpu::BufferView(new gpu::Buffer(sizeof(part), (gpu::Byte*) &part), gpu::Element::PART_DRAWCALL));

        _spotLightMesh->makeBufferStream();
    }
    return _spotLightMesh;
}

