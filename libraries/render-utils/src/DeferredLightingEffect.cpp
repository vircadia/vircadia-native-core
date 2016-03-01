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

#include "AbstractViewStateInterface.h"
#include "GeometryCache.h"
#include "TextureCache.h"
#include "FramebufferCache.h"

#include "deferred_light_vert.h"
#include "deferred_light_limited_vert.h"
#include "deferred_light_spot_vert.h"

#include "directional_light_frag.h"
#include "directional_ambient_light_frag.h"
#include "directional_skybox_light_frag.h"

#include "directional_light_shadow_frag.h"
#include "directional_ambient_light_shadow_frag.h"
#include "directional_skybox_light_shadow_frag.h"

#include "point_light_frag.h"
#include "spot_light_frag.h"

struct LightLocations {
    int radius;
    int ambientSphere;
    int lightBufferUnit;
    int texcoordMat;
    int coneParam;
    int deferredTransformBuffer;
    int shadowTransformBuffer;
};

enum {
    DEFERRED_BUFFER_COLOR_UNIT = 0,
    DEFERRED_BUFFER_NORMAL_UNIT = 1,
    DEFERRED_BUFFER_EMISSIVE_UNIT = 2,
    DEFERRED_BUFFER_DEPTH_UNIT = 3,
    DEFERRED_BUFFER_OBSCURANCE_UNIT = 4,
    SHADOW_MAP_UNIT = 5,
    SKYBOX_MAP_UNIT = 6,
};
static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& program, LightLocationsPtr& locations);

void DeferredLightingEffect::init() {
    _directionalLightLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightLocations = std::make_shared<LightLocations>();

    _directionalLightShadowLocations = std::make_shared<LightLocations>();
    _directionalAmbientSphereLightShadowLocations = std::make_shared<LightLocations>();
    _directionalSkyboxLightShadowLocations = std::make_shared<LightLocations>();

    _pointLightLocations = std::make_shared<LightLocations>();
    _spotLightLocations = std::make_shared<LightLocations>();

    loadLightProgram(deferred_light_vert, directional_light_frag, false, _directionalLight, _directionalLightLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_frag, false, _directionalAmbientSphereLight, _directionalAmbientSphereLightLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_frag, false, _directionalSkyboxLight, _directionalSkyboxLightLocations);

    loadLightProgram(deferred_light_vert, directional_light_shadow_frag, false, _directionalLightShadow, _directionalLightShadowLocations);
    loadLightProgram(deferred_light_vert, directional_ambient_light_shadow_frag, false, _directionalAmbientSphereLightShadow, _directionalAmbientSphereLightShadowLocations);
    loadLightProgram(deferred_light_vert, directional_skybox_light_shadow_frag, false, _directionalSkyboxLightShadow, _directionalSkyboxLightShadowLocations);

    loadLightProgram(deferred_light_limited_vert, point_light_frag, true, _pointLight, _pointLightLocations);
    loadLightProgram(deferred_light_spot_vert, spot_light_frag, true, _spotLight, _spotLightLocations);

    // Allocate a global light representing the Global Directional light casting shadow (the sun) and the ambient light
    _globalLights.push_back(0);
    _allocatedLights.push_back(std::make_shared<model::Light>());

    model::LightPointer lp = _allocatedLights[0];
    lp->setType(model::Light::SUN);

    // Add the global light to the light stage (for later shadow rendering)
    _lightStage.addLight(lp);

    lp->setDirection(glm::vec3(-1.0f));
    lp->setColor(glm::vec3(1.0f));
    lp->setIntensity(1.0f);
    lp->setType(model::Light::SUN);
    lp->setAmbientSpherePreset(gpu::SphericalHarmonics::Preset::OLD_TOWN_SQUARE);
}

void DeferredLightingEffect::addPointLight(const glm::vec3& position, float radius, const glm::vec3& color,
        float intensity, float falloffRadius) {
    addSpotLight(position, radius, color, intensity, falloffRadius);
}

void DeferredLightingEffect::addSpotLight(const glm::vec3& position, float radius, const glm::vec3& color,
    float intensity, float falloffRadius, const glm::quat& orientation, float exponent, float cutoff) {
    
    unsigned int lightID = (unsigned int)(_pointLights.size() + _spotLights.size() + _globalLights.size());
    if (lightID >= _allocatedLights.size()) {
        _allocatedLights.push_back(std::make_shared<model::Light>());
    }
    model::LightPointer lp = _allocatedLights[lightID];

    lp->setPosition(position);
    lp->setMaximumRadius(radius);
    lp->setColor(color);
    lp->setIntensity(intensity);
    lp->setFalloffRadius(falloffRadius);

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
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        // Clear Lighting buffer
        auto lightingFbo = DependencyManager::get<FramebufferCache>()->getLightingFramebuffer();

        batch.setFramebuffer(lightingFbo);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR0, vec4(vec3(0), 0), true);

        // Clear deferred
        auto deferredFbo = DependencyManager::get<FramebufferCache>()->getDeferredFramebuffer();

        batch.setFramebuffer(deferredFbo);

        // Clear Color, Depth and Stencil
        batch.clearFramebuffer(
            gpu::Framebuffer::BUFFER_COLOR0 |
            gpu::Framebuffer::BUFFER_DEPTH |
            gpu::Framebuffer::BUFFER_STENCIL,
            vec4(vec3(0), 1), 1.0, 0.0, true);

        // clear the normal and specular buffers
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), true);
        const float MAX_SPECULAR_EXPONENT = 128.0f;
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR2, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f / MAX_SPECULAR_EXPONENT), true);
    });
}

void DeferredLightingEffect::render(const render::RenderContextPointer& renderContext) {
    auto args = renderContext->args;
    gpu::doInBatch(args->_context, [&](gpu::Batch& batch) {
        
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
        auto textureCache = DependencyManager::get<TextureCache>();
    
        QSize framebufferSize = framebufferCache->getFrameBufferSize();
    
        // binding the first framebuffer
        auto lightingFBO = framebufferCache->getLightingFramebuffer();
        batch.setFramebuffer(lightingFBO);

        batch.setViewportTransform(args->_viewport);
        batch.setStateScissorRect(args->_viewport);

        // Bind the G-Buffer surfaces
        batch.setResourceTexture(DEFERRED_BUFFER_COLOR_UNIT, framebufferCache->getDeferredColorTexture());
        batch.setResourceTexture(DEFERRED_BUFFER_NORMAL_UNIT, framebufferCache->getDeferredNormalTexture());
        batch.setResourceTexture(DEFERRED_BUFFER_EMISSIVE_UNIT, framebufferCache->getDeferredSpecularTexture());
        batch.setResourceTexture(DEFERRED_BUFFER_DEPTH_UNIT, framebufferCache->getPrimaryDepthTexture());

        // FIXME: Different render modes should have different tasks
        if (args->_renderMode == RenderArgs::DEFAULT_RENDER_MODE && _ambientOcclusionEnabled) {
            batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, framebufferCache->getOcclusionTexture());
        } else {
            // need to assign the white texture if ao is off
            batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, textureCache->getWhiteTexture());
        }

        assert(_lightStage.lights.size() > 0);
        const auto& globalShadow = _lightStage.lights[0]->shadow;

        // Bind the shadow buffer
        batch.setResourceTexture(SHADOW_MAP_UNIT, globalShadow.map);

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
                auto& program = _shadowMapEnabled ? _directionalLightShadow : _directionalLight;
                LightLocationsPtr locations = _shadowMapEnabled ? _directionalLightShadowLocations : _directionalLightLocations;
                const auto& keyLight = _allocatedLights[_globalLights.front()];

                // Setup the global directional pass pipeline
                {
                    if (_shadowMapEnabled) {
                        if (keyLight->getAmbientMap()) {
                            program = _directionalSkyboxLightShadow;
                            locations = _directionalSkyboxLightShadowLocations;
                        } else {
                            program = _directionalAmbientSphereLightShadow;
                            locations = _directionalAmbientSphereLightShadowLocations;
                        }
                    } else {
                        if (keyLight->getAmbientMap()) {
                            program = _directionalSkyboxLight;
                            locations = _directionalSkyboxLightLocations;
                        } else {
                            program = _directionalAmbientSphereLight;
                            locations = _directionalAmbientSphereLightLocations;
                        }
                    }

                    if (locations->shadowTransformBuffer >= 0) {
                        batch.setUniformBuffer(locations->shadowTransformBuffer, globalShadow.getBuffer());
                    }
                    batch.setPipeline(program);
                }

                { // Setup the global lighting
                    setupKeyLightBatch(batch, locations->lightBufferUnit, SKYBOX_MAP_UNIT);
                }

                {
                    batch.setModelTransform(Transform());
                    batch.setProjectionTransform(glm::mat4());
                    batch.setViewTransform(Transform());

                    glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
                   geometryCache->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
                }

                if (keyLight->getAmbientMap()) {
                    batch.setResourceTexture(SKYBOX_MAP_UNIT, nullptr);
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
        batch.setResourceTexture(DEFERRED_BUFFER_COLOR_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_NORMAL_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_EMISSIVE_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_DEPTH_UNIT, nullptr);
        batch.setResourceTexture(DEFERRED_BUFFER_OBSCURANCE_UNIT, nullptr);
        batch.setResourceTexture(SHADOW_MAP_UNIT, nullptr);
        batch.setResourceTexture(SKYBOX_MAP_UNIT, nullptr);

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

void DeferredLightingEffect::setupKeyLightBatch(gpu::Batch& batch, int lightBufferUnit, int skyboxCubemapUnit) {
    PerformanceTimer perfTimer("DLE->setupBatch()");
    auto keyLight = _allocatedLights[_globalLights.front()];

    if (lightBufferUnit >= 0) {
        batch.setUniformBuffer(lightBufferUnit, keyLight->getSchemaBuffer());
    }

    if (keyLight->getAmbientMap() && (skyboxCubemapUnit >= 0)) {
        batch.setResourceTexture(skyboxCubemapUnit, keyLight->getAmbientMap());
    }
}

static void loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& pipeline, LightLocationsPtr& locations) {
    auto VS = gpu::Shader::createVertex(std::string(vertSource));
    auto PS = gpu::Shader::createPixel(std::string(fragSource));
    
    gpu::ShaderPointer program = gpu::Shader::createProgram(VS, PS);

    gpu::Shader::BindingSet slotBindings;
    slotBindings.insert(gpu::Shader::Binding(std::string("colorMap"), DEFERRED_BUFFER_COLOR_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("normalMap"), DEFERRED_BUFFER_NORMAL_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("specularMap"), DEFERRED_BUFFER_EMISSIVE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("depthMap"), DEFERRED_BUFFER_DEPTH_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("obscuranceMap"), DEFERRED_BUFFER_OBSCURANCE_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("shadowMap"), SHADOW_MAP_UNIT));
    slotBindings.insert(gpu::Shader::Binding(std::string("skyboxMap"), SKYBOX_MAP_UNIT));

    static const int LIGHT_GPU_SLOT = 3;
    static const int DEFERRED_TRANSFORM_BUFFER_SLOT = 2;
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), LIGHT_GPU_SLOT));
    slotBindings.insert(gpu::Shader::Binding(std::string("deferredTransformBuffer"), DEFERRED_TRANSFORM_BUFFER_SLOT));

    gpu::Shader::makeProgram(*program, slotBindings);

    locations->radius = program->getUniforms().findLocation("radius");
    locations->ambientSphere = program->getUniforms().findLocation("ambientSphere.L00");

    locations->texcoordMat = program->getUniforms().findLocation("texcoordMat");
    locations->coneParam = program->getUniforms().findLocation("coneParam");

    locations->lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
    locations->deferredTransformBuffer = program->getBuffers().findLocation("deferredTransformBuffer");
    locations->shadowTransformBuffer = program->getBuffers().findLocation("shadowTransformBuffer");

    auto state = std::make_shared<gpu::State>();
    state->setColorWriteMask(true, true, true, false);

    // Stencil test all the light passes for objects pixels only, not the background
    state->setStencilTest(true, 0xFF, gpu::State::StencilTest(0, 0xFF, gpu::NOT_EQUAL, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP, gpu::State::STENCIL_OP_KEEP));

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
    pipeline = gpu::Pipeline::create(program, state);

}

void DeferredLightingEffect::setGlobalLight(const model::LightPointer& light) {
    auto globalLight = _allocatedLights.front();
    globalLight->setDirection(light->getDirection());
    globalLight->setColor(light->getColor());
    globalLight->setIntensity(light->getIntensity());
    globalLight->setAmbientIntensity(light->getAmbientIntensity());
    globalLight->setAmbientSphere(light->getAmbientSphere());
    globalLight->setAmbientMap(light->getAmbientMap());
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
    }
    return _spotLightMesh;
}

