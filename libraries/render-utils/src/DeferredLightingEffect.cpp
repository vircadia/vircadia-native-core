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

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include <gpu/GPUConfig.h>

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <ViewFrustum.h>

#include "AbstractViewStateInterface.h"
#include "DeferredLightingEffect.h"
#include "GeometryCache.h"
#include "GlowEffect.h"
#include "RenderUtil.h"
#include "TextureCache.h"

#include "gpu/Batch.h"
#include "gpu/GLBackend.h"

#include "simple_vert.h"
#include "simple_textured_frag.h"
#include "simple_textured_emisive_frag.h"

#include "deferred_light_vert.h"
#include "deferred_light_limited_vert.h"

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

void DeferredLightingEffect::init(AbstractViewStateInterface* viewState) {
    auto VS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(simple_vert)));
    auto PS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(simple_textured_frag)));
    auto PSEmissive = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(simple_textured_emisive_frag)));
    
    gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(VS, PS));
    gpu::ShaderPointer programEmissive = gpu::ShaderPointer(gpu::Shader::createProgram(VS, PSEmissive));
    
    gpu::Shader::BindingSet slotBindings;
    gpu::Shader::makeProgram(*program, slotBindings);
    gpu::Shader::makeProgram(*programEmissive, slotBindings);
    
    gpu::StatePointer state = gpu::StatePointer(new gpu::State());
    state->setCullMode(gpu::State::CULL_BACK);
    state->setDepthTest(true, true, gpu::LESS_EQUAL);
    state->setBlendFunction(false,
                            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    
    
    gpu::StatePointer stateCullNone = gpu::StatePointer(new gpu::State());
    stateCullNone->setCullMode(gpu::State::CULL_NONE);
    stateCullNone->setDepthTest(true, true, gpu::LESS_EQUAL);
    stateCullNone->setBlendFunction(false,
                            gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                            gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
    
    _simpleProgram = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    _simpleProgramCullNone = gpu::PipelinePointer(gpu::Pipeline::create(program, stateCullNone));
    _simpleProgramEmissive = gpu::PipelinePointer(gpu::Pipeline::create(programEmissive, state));
    _simpleProgramEmissiveCullNone = gpu::PipelinePointer(gpu::Pipeline::create(programEmissive, stateCullNone));

    _viewState = viewState;
    loadLightProgram(directional_light_frag, false, _directionalLight, _directionalLightLocations);
    loadLightProgram(directional_light_shadow_map_frag, false, _directionalLightShadowMap,
        _directionalLightShadowMapLocations);
    loadLightProgram(directional_light_cascaded_shadow_map_frag, false, _directionalLightCascadedShadowMap,
        _directionalLightCascadedShadowMapLocations);

    loadLightProgram(directional_ambient_light_frag, false, _directionalAmbientSphereLight, _directionalAmbientSphereLightLocations);
    loadLightProgram(directional_ambient_light_shadow_map_frag, false, _directionalAmbientSphereLightShadowMap,
        _directionalAmbientSphereLightShadowMapLocations);
    loadLightProgram(directional_ambient_light_cascaded_shadow_map_frag, false, _directionalAmbientSphereLightCascadedShadowMap,
        _directionalAmbientSphereLightCascadedShadowMapLocations);

    loadLightProgram(directional_skybox_light_frag, false, _directionalSkyboxLight, _directionalSkyboxLightLocations);
    loadLightProgram(directional_skybox_light_shadow_map_frag, false, _directionalSkyboxLightShadowMap,
        _directionalSkyboxLightShadowMapLocations);
    loadLightProgram(directional_skybox_light_cascaded_shadow_map_frag, false, _directionalSkyboxLightCascadedShadowMap,
        _directionalSkyboxLightCascadedShadowMapLocations);

    loadLightProgram(point_light_frag, true, _pointLight, _pointLightLocations);
    loadLightProgram(spot_light_frag, true, _spotLight, _spotLightLocations);

    // Allocate a global light representing the Global Directional light casting shadow (the sun) and the ambient light
    _globalLights.push_back(0);
    _allocatedLights.push_back(model::LightPointer(new model::Light()));

    model::LightPointer lp = _allocatedLights[0];

    lp->setDirection(-glm::vec3(1.0f, 1.0f, 1.0f));
    lp->setColor(glm::vec3(1.0f));
    lp->setIntensity(1.0f);
    lp->setType(model::Light::SUN);
    lp->setAmbientSpherePreset(gpu::SphericalHarmonics::Preset(_ambientLightMode % gpu::SphericalHarmonics::NUM_PRESET));
}

void DeferredLightingEffect::bindSimpleProgram(gpu::Batch& batch, bool textured, bool culled, bool emmisive) {
    if (emmisive) {
        if (culled) {
            batch.setPipeline(_simpleProgramEmissive);
        } else {
            batch.setPipeline(_simpleProgramEmissiveCullNone);
        }
    } else {
        if (culled) {
            batch.setPipeline(_simpleProgram);
        } else {
            batch.setPipeline(_simpleProgramCullNone);
        }
    }
    if (!textured) {
        // If it is not textured, bind white texture and keep using textured pipeline
        batch.setUniformTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
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
        _allocatedLights.push_back(model::LightPointer(new model::Light()));
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
    // clear the normal and specular buffers
    auto textureCache = DependencyManager::get<TextureCache>();
    textureCache->setPrimaryDrawBuffers(false, true, false);
    glClear(GL_COLOR_BUFFER_BIT);
    textureCache->setPrimaryDrawBuffers(false, false, true);
    // clearing to zero alpha for specular causes problems on my Nvidia card; clear to lowest non-zero value instead
    const float MAX_SPECULAR_EXPONENT = 128.0f;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f / MAX_SPECULAR_EXPONENT);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    textureCache->setPrimaryDrawBuffers(true, false, false);
}

void DeferredLightingEffect::render(RenderArgs* args) {
    // perform deferred lighting, rendering to free fbo
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_COLOR_MATERIAL);
    glDepthMask(false);

    auto textureCache = DependencyManager::get<TextureCache>();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0 );

    QSize framebufferSize = textureCache->getFrameBufferSize();
    
    // binding the first framebuffer
    auto freeFBO = DependencyManager::get<GlowEffect>()->getFreeFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(freeFBO));
 
    glClear(GL_COLOR_BUFFER_BIT);
   // glEnable(GL_FRAMEBUFFER_SRGB);

   // glBindTexture(GL_TEXTURE_2D, primaryFBO->texture());
    glBindTexture(GL_TEXTURE_2D, textureCache->getPrimaryColorTextureID());
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureCache->getPrimaryNormalTextureID());
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureCache->getPrimarySpecularTextureID());
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, textureCache->getPrimaryDepthTextureID());
        
    // get the viewport side (left, right, both)
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_X_INDEX = 0;
    const int VIEWPORT_Y_INDEX = 1;
    const int VIEWPORT_WIDTH_INDEX = 2;
    const int VIEWPORT_HEIGHT_INDEX = 3;

    float sMin = viewport[VIEWPORT_X_INDEX] / (float)framebufferSize.width();
    float sWidth = viewport[VIEWPORT_WIDTH_INDEX] / (float)framebufferSize.width();
    float tMin = viewport[VIEWPORT_Y_INDEX] / (float)framebufferSize.height();
    float tHeight = viewport[VIEWPORT_HEIGHT_INDEX] / (float)framebufferSize.height();

    bool useSkyboxCubemap = (_skybox) && (_skybox->getCubemap());

    // Fetch the ViewMatrix;
    glm::mat4 invViewMat;
    _viewState->getViewTransform().getMatrix(invViewMat);

    ProgramObject* program = &_directionalLight;
    const LightLocations* locations = &_directionalLightLocations;
    bool shadowsEnabled = _viewState->getShadowsEnabled();
    if (shadowsEnabled) {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, textureCache->getShadowDepthTextureID());
        
        program = &_directionalLightShadowMap;
        locations = &_directionalLightShadowMapLocations;
        if (_viewState->getCascadeShadowsEnabled()) {
            program = &_directionalLightCascadedShadowMap;
            locations = &_directionalLightCascadedShadowMapLocations;
            if (useSkyboxCubemap) {
                program = &_directionalSkyboxLightCascadedShadowMap;
                locations = &_directionalSkyboxLightCascadedShadowMapLocations;
            } else if (_ambientLightMode > -1) {
                program = &_directionalAmbientSphereLightCascadedShadowMap;
                locations = &_directionalAmbientSphereLightCascadedShadowMapLocations;
            }
            program->bind();
            program->setUniform(locations->shadowDistances, _viewState->getShadowDistances());
        
        } else {
            if (useSkyboxCubemap) {
                program = &_directionalSkyboxLightShadowMap;
                locations = &_directionalSkyboxLightShadowMapLocations;
            } else if (_ambientLightMode > -1) {
                program = &_directionalAmbientSphereLightShadowMap;
                locations = &_directionalAmbientSphereLightShadowMapLocations;
            }
            program->bind();
        }
        program->setUniformValue(locations->shadowScale,
            1.0f / textureCache->getShadowFramebuffer()->getWidth());
        
    } else {
        if (useSkyboxCubemap) {
                program = &_directionalSkyboxLight;
                locations = &_directionalSkyboxLightLocations;
        } else if (_ambientLightMode > -1) {
            program = &_directionalAmbientSphereLight;
            locations = &_directionalAmbientSphereLightLocations;
        }
        program->bind();
    }

    {
        auto globalLight = _allocatedLights[_globalLights.front()];
    
        if (locations->ambientSphere >= 0) {
            gpu::SphericalHarmonics sh = globalLight->getAmbientSphere();
            if (useSkyboxCubemap && _skybox->getCubemap()->getIrradiance()) {
                sh = (*_skybox->getCubemap()->getIrradiance());
            }
            for (int i =0; i <gpu::SphericalHarmonics::NUM_COEFFICIENTS; i++) {
                program->setUniformValue(locations->ambientSphere + i, *(((QVector4D*) &sh) + i)); 
            }
        }
    
        if (useSkyboxCubemap) {
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, gpu::GLBackend::getTextureID(_skybox->getCubemap()));
        }

        if (locations->lightBufferUnit >= 0) {
            gpu::Batch batch;
            batch.setUniformBuffer(locations->lightBufferUnit, globalLight->getSchemaBuffer());
            gpu::GLBackend::renderBatch(batch);
        }
        
        if (_atmosphere && (locations->atmosphereBufferUnit >= 0)) {
            gpu::Batch batch;
            batch.setUniformBuffer(locations->atmosphereBufferUnit, _atmosphere->getDataBuffer());
            gpu::GLBackend::renderBatch(batch);
        }
        glUniformMatrix4fv(locations->invViewMat, 1, false, reinterpret_cast< const GLfloat* >(&invViewMat));
    }

    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    _viewState->computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    program->setUniformValue(locations->nearLocation, nearVal);
    float depthScale = (farVal - nearVal) / farVal;
    program->setUniformValue(locations->depthScale, depthScale);
    float nearScale = -1.0f / nearVal;
    float depthTexCoordScaleS = (right - left) * nearScale / sWidth;
    float depthTexCoordScaleT = (top - bottom) * nearScale / tHeight;
    float depthTexCoordOffsetS = left * nearScale - sMin * depthTexCoordScaleS;
    float depthTexCoordOffsetT = bottom * nearScale - tMin * depthTexCoordScaleT;
    program->setUniformValue(locations->depthTexCoordOffset, depthTexCoordOffsetS, depthTexCoordOffsetT);
    program->setUniformValue(locations->depthTexCoordScale, depthTexCoordScaleS, depthTexCoordScaleT);
    
    renderFullscreenQuad(sMin, sMin + sWidth, tMin, tMin + tHeight);
    
    program->release();

    if (useSkyboxCubemap) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        if (!shadowsEnabled) {
            glActiveTexture(GL_TEXTURE3);
        }
    }

    if (shadowsEnabled) {
        glBindTexture(GL_TEXTURE_2D, 0);        
        glActiveTexture(GL_TEXTURE3);
    }
    
    // additive blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    
    glEnable(GL_CULL_FACE);
    
    glm::vec4 sCoefficients(sWidth / 2.0f, 0.0f, 0.0f, sMin + sWidth / 2.0f);
    glm::vec4 tCoefficients(0.0f, tHeight / 2.0f, 0.0f, tMin + tHeight / 2.0f);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, (const GLfloat*)&sCoefficients);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, (const GLfloat*)&tCoefficients);
    
    // enlarge the scales slightly to account for tesselation
    const float SCALE_EXPANSION = 0.05f;
    
    const glm::vec3& eyePoint = _viewState->getCurrentViewFrustum()->getPosition();
    float nearRadius = glm::distance(eyePoint, _viewState->getCurrentViewFrustum()->getNearTopLeft());

    auto geometryCache = DependencyManager::get<GeometryCache>();
    
    if (!_pointLights.empty()) {
        _pointLight.bind();
        _pointLight.setUniformValue(_pointLightLocations.nearLocation, nearVal);
        _pointLight.setUniformValue(_pointLightLocations.depthScale, depthScale);
        _pointLight.setUniformValue(_pointLightLocations.depthTexCoordOffset, depthTexCoordOffsetS, depthTexCoordOffsetT);
        _pointLight.setUniformValue(_pointLightLocations.depthTexCoordScale, depthTexCoordScaleS, depthTexCoordScaleT);

        for (auto lightID : _pointLights) {
            auto light = _allocatedLights[lightID];

            if (_pointLightLocations.lightBufferUnit >= 0) {
                gpu::Batch batch;
                batch.setUniformBuffer(_pointLightLocations.lightBufferUnit, light->getSchemaBuffer());
                gpu::GLBackend::renderBatch(batch);
            }
            glUniformMatrix4fv(_pointLightLocations.invViewMat, 1, false, reinterpret_cast< const GLfloat* >(&invViewMat));

            glPushMatrix();
            
            float expandedRadius = light->getMaximumRadius() * (1.0f + SCALE_EXPANSION);
            if (glm::distance(eyePoint, glm::vec3(light->getPosition())) < expandedRadius + nearRadius) {
                glLoadIdentity();
                glTranslatef(0.0f, 0.0f, -1.0f);
                
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadIdentity();
                
                renderFullscreenQuad();
            
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                
            } else {
                glTranslatef(light->getPosition().x, light->getPosition().y, light->getPosition().z);   
                geometryCache->renderSphere(expandedRadius, 32, 32, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
            }
            
            glPopMatrix();
        }
        _pointLights.clear();
        
        _pointLight.release();
    }
    
    if (!_spotLights.empty()) {
        _spotLight.bind();
        _spotLight.setUniformValue(_spotLightLocations.nearLocation, nearVal);
        _spotLight.setUniformValue(_spotLightLocations.depthScale, depthScale);
        _spotLight.setUniformValue(_spotLightLocations.depthTexCoordOffset, depthTexCoordOffsetS, depthTexCoordOffsetT);
        _spotLight.setUniformValue(_spotLightLocations.depthTexCoordScale, depthTexCoordScaleS, depthTexCoordScaleT);
        
        for (auto lightID : _spotLights) {
            auto light = _allocatedLights[lightID];

            if (_spotLightLocations.lightBufferUnit >= 0) {
                gpu::Batch batch;
                batch.setUniformBuffer(_spotLightLocations.lightBufferUnit, light->getSchemaBuffer());
                gpu::GLBackend::renderBatch(batch);
            }
            glUniformMatrix4fv(_spotLightLocations.invViewMat, 1, false, reinterpret_cast< const GLfloat* >(&invViewMat));

            glPushMatrix();
            
            float expandedRadius = light->getMaximumRadius() * (1.0f + SCALE_EXPANSION);
            float edgeRadius = expandedRadius / glm::cos(light->getSpotAngle());
            if (glm::distance(eyePoint, glm::vec3(light->getPosition())) < edgeRadius + nearRadius) {
                glLoadIdentity();
                glTranslatef(0.0f, 0.0f, -1.0f);
                
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadIdentity();
                
                renderFullscreenQuad();
                
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                
            } else {
                glTranslatef(light->getPosition().x, light->getPosition().y, light->getPosition().z);
                glm::quat spotRotation = rotationBetween(glm::vec3(0.0f, 0.0f, -1.0f), light->getDirection());
                glm::vec3 axis = glm::axis(spotRotation);
                glRotatef(glm::degrees(glm::angle(spotRotation)), axis.x, axis.y, axis.z);   
                glTranslatef(0.0f, 0.0f, -light->getMaximumRadius() * (1.0f + SCALE_EXPANSION * 0.5f));  
                geometryCache->renderCone(expandedRadius * glm::tan(light->getSpotAngle()),
                    expandedRadius, 32, 1);
            }
            
            glPopMatrix();
        }
        _spotLights.clear();
        
        _spotLight.release();
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
        
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
  //  glDisable(GL_FRAMEBUFFER_SRGB);
    
    // End of the Lighting pass
}

void DeferredLightingEffect::copyBack(RenderArgs* args) {
    auto textureCache = DependencyManager::get<TextureCache>();
    QSize framebufferSize = textureCache->getFrameBufferSize();

    auto freeFBO = DependencyManager::get<GlowEffect>()->getFreeFramebuffer();

    //freeFBO->release();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    
    glDisable(GL_CULL_FACE);
    
    // now transfer the lit region to the primary fbo
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glColorMask(true, true, true, false);
    
    auto primaryFBO = gpu::GLBackend::getFramebufferID(textureCache->getPrimaryFramebuffer());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, primaryFBO);

    //primaryFBO->bind();
    
    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(freeFBO->getRenderBuffer(0)));
    glEnable(GL_TEXTURE_2D);
    
    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_X_INDEX = 0;
    const int VIEWPORT_Y_INDEX = 1;
    const int VIEWPORT_WIDTH_INDEX = 2;
    const int VIEWPORT_HEIGHT_INDEX = 3;

    float sMin = viewport[VIEWPORT_X_INDEX] / (float)framebufferSize.width();
    float sWidth = viewport[VIEWPORT_WIDTH_INDEX] / (float)framebufferSize.width();
    float tMin = viewport[VIEWPORT_Y_INDEX] / (float)framebufferSize.height();
    float tHeight = viewport[VIEWPORT_HEIGHT_INDEX] / (float)framebufferSize.height();

    renderFullscreenQuad(sMin, sMin + sWidth, tMin, tMin + tHeight);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    
    glColorMask(true, true, true, true);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void DeferredLightingEffect::setupTransparent(RenderArgs* args, int lightBufferUnit) {
    auto globalLight = _allocatedLights[_globalLights.front()];
    args->_batch->setUniformBuffer(lightBufferUnit, globalLight->getSchemaBuffer());
}

void DeferredLightingEffect::loadLightProgram(const char* fragSource, bool limited, ProgramObject& program, LightLocations& locations) {
    program.addShaderFromSourceCode(QGLShader::Vertex, (limited ? deferred_light_limited_vert : deferred_light_vert));
    program.addShaderFromSourceCode(QGLShader::Fragment, fragSource);
    program.link();
    
    program.bind();
    program.setUniformValue("diffuseMap", 0);
    program.setUniformValue("normalMap", 1);
    program.setUniformValue("specularMap", 2);
    program.setUniformValue("depthMap", 3);
    program.setUniformValue("shadowMap", 4);
    program.setUniformValue("skyboxMap", 5);
    locations.shadowDistances = program.uniformLocation("shadowDistances");
    locations.shadowScale = program.uniformLocation("shadowScale");
    locations.nearLocation = program.uniformLocation("near");
    locations.depthScale = program.uniformLocation("depthScale");
    locations.depthTexCoordOffset = program.uniformLocation("depthTexCoordOffset");
    locations.depthTexCoordScale = program.uniformLocation("depthTexCoordScale");
    locations.radius = program.uniformLocation("radius");
    locations.ambientSphere = program.uniformLocation("ambientSphere.L00");
    locations.invViewMat = program.uniformLocation("invViewMat");

    GLint loc = -1;

#if (GPU_FEATURE_PROFILE == GPU_CORE)
    const GLint LIGHT_GPU_SLOT = 3;
    loc = glGetUniformBlockIndex(program.programId(), "lightBuffer");
    if (loc >= 0) {
        glUniformBlockBinding(program.programId(), loc, LIGHT_GPU_SLOT);
        locations.lightBufferUnit = LIGHT_GPU_SLOT;
    } else {
        locations.lightBufferUnit = -1;
    }
#else
    loc = program.uniformLocation("lightBuffer");
    if (loc >= 0) {
        locations.lightBufferUnit = loc;
    } else {
        locations.lightBufferUnit = -1;
    }
#endif

#if (GPU_FEATURE_PROFILE == GPU_CORE)
    const GLint ATMOSPHERE_GPU_SLOT = 4;
    loc = glGetUniformBlockIndex(program.programId(), "atmosphereBufferUnit");
    if (loc >= 0) {
        glUniformBlockBinding(program.programId(), loc, ATMOSPHERE_GPU_SLOT);
        locations.atmosphereBufferUnit = ATMOSPHERE_GPU_SLOT;
    } else {
        locations.atmosphereBufferUnit = -1;
    }
#else
    loc = program.uniformLocation("atmosphereBufferUnit");
    if (loc >= 0) {
        locations.atmosphereBufferUnit = loc;
    } else {
        locations.atmosphereBufferUnit = -1;
    }
#endif

    program.release();
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
