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
#include <gpu/GPUConfig.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/StandardShaderLib.h>
#include <PathUtils.h>
#include <ViewFrustum.h>

#include "AbstractViewStateInterface.h"
#include "GeometryCache.h"
#include "RenderUtil.h"
#include "TextureCache.h"


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
    gpu::Shader::makeProgram(*_simpleShader, slotBindings);
    gpu::Shader::makeProgram(*_emissiveShader, slotBindings);

    _viewState = viewState;
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

void DeferredLightingEffect::bindSimpleProgram(gpu::Batch& batch, bool textured, bool culled,
                                               bool emmisive, bool depthBias) {
    SimpleProgramKey config{textured, culled, emmisive, depthBias};
    batch.setPipeline(getPipeline(config));
    
    if (!config.isTextured()) {
        // If it is not textured, bind white texture and keep using textured pipeline
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }
}

void DeferredLightingEffect::renderSolidSphere(gpu::Batch& batch, float radius, int slices, int stacks, const glm::vec4& color) {
    bindSimpleProgram(batch);
    DependencyManager::get<GeometryCache>()->renderSphere(batch, radius, slices, stacks, color);
}

void DeferredLightingEffect::renderWireSphere(gpu::Batch& batch, float radius, int slices, int stacks, const glm::vec4& color) {
    bindSimpleProgram(batch);
    DependencyManager::get<GeometryCache>()->renderSphere(batch, radius, slices, stacks, color, false);
}

void DeferredLightingEffect::renderSolidCube(gpu::Batch& batch, float size, const glm::vec4& color) {
    bindSimpleProgram(batch);
    DependencyManager::get<GeometryCache>()->renderSolidCube(batch, size, color);
}

void DeferredLightingEffect::renderWireCube(gpu::Batch& batch, float size, const glm::vec4& color) {
    bindSimpleProgram(batch);
    DependencyManager::get<GeometryCache>()->renderWireCube(batch, size, color);
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

    auto textureCache = DependencyManager::get<TextureCache>();
    gpu::Batch batch;
    
    // clear the normal and specular buffers
    batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    const float MAX_SPECULAR_EXPONENT = 128.0f;
    batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLOR2, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f / MAX_SPECULAR_EXPONENT));

    args->_context->syncCache();
    args->_context->render(batch);
}

void DeferredLightingEffect::render(RenderArgs* args) {
    gpu::Batch batch;

    // perform deferred lighting, rendering to free fbo
    auto textureCache = DependencyManager::get<TextureCache>();
    
    QSize framebufferSize = textureCache->getFrameBufferSize();
    
    // binding the first framebuffer
    auto freeFBO = DependencyManager::get<TextureCache>()->getSecondaryFramebuffer();
    batch.setFramebuffer(freeFBO);

    batch.setViewportTransform(args->_viewport);
 
    batch.clearColorFramebuffer(freeFBO->getBufferMask(), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    
    batch.setResourceTexture(0, textureCache->getPrimaryColorTexture());

    batch.setResourceTexture(1, textureCache->getPrimaryNormalTexture());
    
    batch.setResourceTexture(2, textureCache->getPrimarySpecularTexture());
    
    batch.setResourceTexture(3, textureCache->getPrimaryDepthTexture());
        
    float sMin = args->_viewport.x / (float)framebufferSize.width();
    float sWidth = args->_viewport.z / (float)framebufferSize.width();
    float tMin = args->_viewport.y / (float)framebufferSize.height();
    float tHeight = args->_viewport.w / (float)framebufferSize.height();

    bool useSkyboxCubemap = (_skybox) && (_skybox->getCubemap());

    // Fetch the ViewMatrix;
    glm::mat4 invViewMat;
    invViewMat = args->_viewFrustum->getView();

    auto& program = _directionalLight;
    const LightLocations* locations = &_directionalLightLocations;
    bool shadowsEnabled = _viewState->getShadowsEnabled();
    if (shadowsEnabled) {
        batch.setResourceTexture(4, textureCache->getShadowFramebuffer()->getDepthStencilBuffer());
        
        program = _directionalLightShadowMap;
        locations = &_directionalLightShadowMapLocations;
        if (_viewState->getCascadeShadowsEnabled()) {
            program = _directionalLightCascadedShadowMap;
            locations = &_directionalLightCascadedShadowMapLocations;
            if (useSkyboxCubemap) {
                program = _directionalSkyboxLightCascadedShadowMap;
                locations = &_directionalSkyboxLightCascadedShadowMapLocations;
            } else if (_ambientLightMode > -1) {
                program = _directionalAmbientSphereLightCascadedShadowMap;
                locations = &_directionalAmbientSphereLightCascadedShadowMapLocations;
            }
            batch.setPipeline(program);
            batch._glUniform3fv(locations->shadowDistances, 1, (const GLfloat*) &_viewState->getShadowDistances());
        
        } else {
            if (useSkyboxCubemap) {
                program = _directionalSkyboxLightShadowMap;
                locations = &_directionalSkyboxLightShadowMapLocations;
            } else if (_ambientLightMode > -1) {
                program = _directionalAmbientSphereLightShadowMap;
                locations = &_directionalAmbientSphereLightShadowMapLocations;
            }
            batch.setPipeline(program);
        }
        batch._glUniform1f(locations->shadowScale, 1.0f / textureCache->getShadowFramebuffer()->getWidth());
        
    } else {
        if (useSkyboxCubemap) {
                program = _directionalSkyboxLight;
                locations = &_directionalSkyboxLightLocations;
        } else if (_ambientLightMode > -1) {
            program = _directionalAmbientSphereLight;
            locations = &_directionalAmbientSphereLightLocations;
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
               batch._glUniform4fv(locations->ambientSphere + i, 1, (const GLfloat*) (&sh) + i * 4);
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
        batch._glUniformMatrix4fv(locations->invViewMat, 1, false, reinterpret_cast< const GLfloat* >(&invViewMat));
    }

    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    args->_viewFrustum->computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

    batch._glUniform1f(locations->nearLocation, nearVal);

    float depthScale = (farVal - nearVal) / farVal;
    batch._glUniform1f(locations->depthScale, depthScale);

    float nearScale = -1.0f / nearVal;
    float depthTexCoordScaleS = (right - left) * nearScale / sWidth;
    float depthTexCoordScaleT = (top - bottom) * nearScale / tHeight;
    float depthTexCoordOffsetS = left * nearScale - sMin * depthTexCoordScaleS;
    float depthTexCoordOffsetT = bottom * nearScale - tMin * depthTexCoordScaleT;
    batch._glUniform2f(locations->depthTexCoordOffset, depthTexCoordOffsetS, depthTexCoordOffsetT);
    batch._glUniform2f(locations->depthTexCoordScale, depthTexCoordScaleS, depthTexCoordScaleT);
    
    {
        Transform model;
        model.setTranslation(glm::vec3(sMin, tMin, 0.0));
        model.setScale(glm::vec3(sWidth, tHeight, 1.0));
        batch.setModelTransform(model);

        batch.setProjectionTransform(glm::mat4());
        batch.setViewTransform(Transform());

        glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec2 topLeft(-1.0f, -1.0f);
        glm::vec2 bottomRight(1.0f, 1.0f);
        glm::vec2 texCoordTopLeft(sMin, tMin);
        glm::vec2 texCoordBottomRight(sMin + sWidth, tMin + tHeight);

        DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
    }

    if (useSkyboxCubemap) {
        batch.setResourceTexture(5, nullptr);
    }

    if (shadowsEnabled) {
        batch.setResourceTexture(4, nullptr);
    }

    glm::vec4 sCoefficients(sWidth / 2.0f, 0.0f, 0.0f, sMin + sWidth / 2.0f);
    glm::vec4 tCoefficients(0.0f, tHeight / 2.0f, 0.0f, tMin + tHeight / 2.0f);
    auto texcoordMat = glm::mat4();
    texcoordMat[0] = glm::vec4(sWidth / 2.0f, 0.0f, 0.0f, sMin + sWidth / 2.0f);
    texcoordMat[1] = glm::vec4(0.0f, tHeight / 2.0f, 0.0f, tMin + tHeight / 2.0f);
    texcoordMat[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    texcoordMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // enlarge the scales slightly to account for tesselation
    const float SCALE_EXPANSION = 0.05f;

    auto eyePoint = args->_viewFrustum->getPosition();
    float nearRadius = glm::distance(eyePoint, args->_viewFrustum->getNearTopLeft());

    auto geometryCache = DependencyManager::get<GeometryCache>();
    
    glm::mat4 projMat;
    Transform viewMat;
    args->_viewFrustum->evalProjectionMatrix(projMat);
    args->_viewFrustum->evalViewTransform(viewMat);
    if (args->_renderMode == RenderArgs::MIRROR_RENDER_MODE) {
        viewMat.postScale(glm::vec3(-1.0f, 1.0f, 1.0f));
    }
    batch.setProjectionTransform(projMat);
    batch.setViewTransform(viewMat);

    if (!_pointLights.empty()) {
        batch.setPipeline(_pointLight);
        batch._glUniform1f(_pointLightLocations.nearLocation, nearVal);
        batch._glUniform1f(_pointLightLocations.depthScale, depthScale);
        batch._glUniform2f(_pointLightLocations.depthTexCoordOffset, depthTexCoordOffsetS, depthTexCoordOffsetT);
        batch._glUniform2f(_pointLightLocations.depthTexCoordScale, depthTexCoordScaleS, depthTexCoordScaleT);
        
        batch._glUniformMatrix4fv(_pointLightLocations.invViewMat, 1, false, reinterpret_cast< const GLfloat* >(&invViewMat));

        batch._glUniformMatrix4fv(_pointLightLocations.texcoordMat, 1, false, reinterpret_cast< const GLfloat* >(&texcoordMat));

        for (auto lightID : _pointLights) {
            auto& light = _allocatedLights[lightID];
            // IN DEBUG: light->setShowContour(true);
            if (_pointLightLocations.lightBufferUnit >= 0) {
                batch.setUniformBuffer(_pointLightLocations.lightBufferUnit, light->getSchemaBuffer());
            }

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
                glm::vec2 topLeft(-1.0f, -1.0f);
                glm::vec2 bottomRight(1.0f, 1.0f);
                glm::vec2 texCoordTopLeft(sMin, tMin);
                glm::vec2 texCoordBottomRight(sMin + sWidth, tMin + tHeight);

                DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
                
                batch.setProjectionTransform(projMat);
                batch.setViewTransform(viewMat);
            } else {
                Transform model;
                model.setTranslation(glm::vec3(light->getPosition().x, light->getPosition().y, light->getPosition().z));
                batch.setModelTransform(model);
                geometryCache->renderSphere(batch, expandedRadius, 32, 32, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
        }
        _pointLights.clear();
    }
    
    if (!_spotLights.empty()) {
        batch.setPipeline(_spotLight);
        batch._glUniform1f(_spotLightLocations.nearLocation, nearVal);
        batch._glUniform1f(_spotLightLocations.depthScale, depthScale);
        batch._glUniform2f(_spotLightLocations.depthTexCoordOffset, depthTexCoordOffsetS, depthTexCoordOffsetT);
        batch._glUniform2f(_spotLightLocations.depthTexCoordScale, depthTexCoordScaleS, depthTexCoordScaleT);
        
        batch._glUniformMatrix4fv(_spotLightLocations.invViewMat, 1, false, reinterpret_cast< const GLfloat* >(&invViewMat));

        batch._glUniformMatrix4fv(_spotLightLocations.texcoordMat, 1, false, reinterpret_cast< const GLfloat* >(&texcoordMat));

        for (auto lightID : _spotLights) {
            auto light = _allocatedLights[lightID];
            // IN DEBUG: light->setShowContour(true);

            batch.setUniformBuffer(_spotLightLocations.lightBufferUnit, light->getSchemaBuffer());

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
                batch._glUniform4fv(_spotLightLocations.coneParam, 1, reinterpret_cast< const GLfloat* >(&coneParam));

                Transform model;
                model.setTranslation(glm::vec3(0.0f, 0.0f, -1.0f));
                batch.setModelTransform(model);
                batch.setViewTransform(Transform());
                batch.setProjectionTransform(glm::mat4());
                
                glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
                glm::vec2 topLeft(-1.0f, -1.0f);
                glm::vec2 bottomRight(1.0f, 1.0f);
                glm::vec2 texCoordTopLeft(sMin, tMin);
                glm::vec2 texCoordBottomRight(sMin + sWidth, tMin + tHeight);

                DependencyManager::get<GeometryCache>()->renderQuad(batch, topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
                
                batch.setProjectionTransform(projMat);
                batch.setViewTransform(viewMat);
            } else {
                coneParam.w = 1.0f;
                batch._glUniform4fv(_spotLightLocations.coneParam, 1, reinterpret_cast< const GLfloat* >(&coneParam));

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
        _spotLights.clear();
    }
    
    // Probably not necessary in the long run because the gpu layer would unbound this texture if used as render target
    batch.setResourceTexture(0, nullptr);
    batch.setResourceTexture(1, nullptr);
    batch.setResourceTexture(2, nullptr);
    batch.setResourceTexture(3, nullptr);

    args->_context->syncCache();
    args->_context->render(batch);

    // End of the Lighting pass
}

void DeferredLightingEffect::copyBack(RenderArgs* args) {
    gpu::Batch batch;
    auto textureCache = DependencyManager::get<TextureCache>();
    QSize framebufferSize = textureCache->getFrameBufferSize();

    auto freeFBO = DependencyManager::get<TextureCache>()->getSecondaryFramebuffer();

    batch.setFramebuffer(textureCache->getPrimaryFramebuffer());
    batch.setPipeline(_blitLightBuffer);
    
    batch.setResourceTexture(0, freeFBO->getRenderBuffer(0));

    batch.setProjectionTransform(glm::mat4());
    batch.setViewTransform(Transform());
    
    float sMin = args->_viewport.x / (float)framebufferSize.width();
    float sWidth = args->_viewport.z / (float)framebufferSize.width();
    float tMin = args->_viewport.y / (float)framebufferSize.height();
    float tHeight = args->_viewport.w / (float)framebufferSize.height();

    batch.setViewportTransform(args->_viewport);

    Transform model;
    model.setTranslation(glm::vec3(sMin, tMin, 0.0));
    model.setScale(glm::vec3(sWidth, tHeight, 1.0));
    batch.setModelTransform(model);

    batch.draw(gpu::TRIANGLE_STRIP, 4);


    args->_context->syncCache();
    args->_context->render(batch);
}

void DeferredLightingEffect::setupTransparent(RenderArgs* args, int lightBufferUnit) {
    auto globalLight = _allocatedLights[_globalLights.front()];
    args->_batch->setUniformBuffer(lightBufferUnit, globalLight->getSchemaBuffer());
}

void DeferredLightingEffect::loadLightProgram(const char* vertSource, const char* fragSource, bool lightVolume, gpu::PipelinePointer& pipeline, LightLocations& locations) {
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
    const GLint LIGHT_GPU_SLOT = 3;
    slotBindings.insert(gpu::Shader::Binding(std::string("lightBuffer"), LIGHT_GPU_SLOT));
    const GLint ATMOSPHERE_GPU_SLOT = 4;
    slotBindings.insert(gpu::Shader::Binding(std::string("atmosphereBufferUnit"), ATMOSPHERE_GPU_SLOT));

    gpu::Shader::makeProgram(*program, slotBindings);

    locations.shadowDistances = program->getUniforms().findLocation("shadowDistances");
    locations.shadowScale = program->getUniforms().findLocation("shadowScale");
    locations.nearLocation = program->getUniforms().findLocation("near");
    locations.depthScale = program->getUniforms().findLocation("depthScale");
    locations.depthTexCoordOffset = program->getUniforms().findLocation("depthTexCoordOffset");
    locations.depthTexCoordScale = program->getUniforms().findLocation("depthTexCoordScale");
    locations.radius = program->getUniforms().findLocation("radius");
    locations.ambientSphere = program->getUniforms().findLocation("ambientSphere.L00");
    locations.invViewMat = program->getUniforms().findLocation("invViewMat");
    locations.texcoordMat = program->getUniforms().findLocation("texcoordMat");
    locations.coneParam = program->getUniforms().findLocation("coneParam");

#if (GPU_FEATURE_PROFILE == GPU_CORE)
    locations.lightBufferUnit = program->getBuffers().findLocation("lightBuffer");
    locations.atmosphereBufferUnit = program->getBuffers().findLocation("atmosphereBufferUnit");
#else
    locations.lightBufferUnit = program->getUniforms().findLocation("lightBuffer");
    locations.atmosphereBufferUnit = program->getUniforms().findLocation("atmosphereBufferUnit");
#endif

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


        GLfloat* vertexData = new GLfloat[verticesSize];
        GLfloat* vertexRing0 = vertexData;
        GLfloat* vertexRing1 = vertexRing0 + ringFloatOffset;
        GLfloat* vertexRing2 = vertexRing1 + ringFloatOffset;
        
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

        _spotLightMesh->setIndexBuffer(gpu::BufferView(new gpu::Buffer(sizeof(GLushort) * indices, (gpu::Byte*) indexData), gpu::Element::INDEX_UINT16));
        delete[] indexData;

        model::Mesh::Part part(0, indices, 0, model::Mesh::TRIANGLES);
        //DEBUG: model::Mesh::Part part(0, indices, 0, model::Mesh::LINE_STRIP);
        
        _spotLightMesh->setPartBuffer(gpu::BufferView(new gpu::Buffer(sizeof(part), (gpu::Byte*) &part), gpu::Element::PART_DRAWCALL));

        _spotLightMesh->makeBufferStream();
    }
    return _spotLightMesh;
}

