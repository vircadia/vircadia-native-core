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
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include "Application.h"
#include "DeferredLightingEffect.h"
#include "RenderUtil.h"

void DeferredLightingEffect::init() {
    loadLightProgram("shaders/directional_light.frag", _directionalLight, _directionalLightLocations);
    loadLightProgram("shaders/directional_light_shadow_map.frag", _directionalLightShadowMap,
        _directionalLightShadowMapLocations);
    loadLightProgram("shaders/directional_light_cascaded_shadow_map.frag", _directionalLightCascadedShadowMap,
        _directionalLightCascadedShadowMapLocations);
}

void DeferredLightingEffect::prepare() {
    // clear the normal and specular buffers
    Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(false, true, true);
    glClear(GL_COLOR_BUFFER_BIT);
    Application::getInstance()->getTextureCache()->setPrimaryDrawBuffers(true, false);
}

void DeferredLightingEffect::render() {
    // perform deferred lighting, rendering to free fbo
    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);    
    
    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_COLOR_MATERIAL);
    glDepthMask(false);
    
    QOpenGLFramebufferObject* primaryFBO = Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject();
    primaryFBO->release();
    
    QOpenGLFramebufferObject* freeFBO = Application::getInstance()->getGlowEffect()->getFreeFramebufferObject();
    freeFBO->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBindTexture(GL_TEXTURE_2D, primaryFBO->texture());
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPrimaryNormalTextureID());
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPrimarySpecularTextureID());
    
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPrimaryDepthTextureID());
        
    // get the viewport side (left, right, both)
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_X_INDEX = 0;
    const int VIEWPORT_Y_INDEX = 1;
    const int VIEWPORT_WIDTH_INDEX = 2;
    const int VIEWPORT_HEIGHT_INDEX = 3;
    float sMin = viewport[VIEWPORT_X_INDEX] / (float)primaryFBO->width();
    float sWidth = viewport[VIEWPORT_WIDTH_INDEX] / (float)primaryFBO->width();
    float tMin = viewport[VIEWPORT_Y_INDEX] / (float)primaryFBO->height();
    float tHeight = viewport[VIEWPORT_HEIGHT_INDEX] / (float)primaryFBO->height();
   
    ProgramObject* program = &_directionalLight;
    const LightLocations* locations = &_directionalLightLocations;
    bool shadowsEnabled = Menu::getInstance()->getShadowsEnabled();
    if (shadowsEnabled) {    
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getShadowDepthTextureID());
        
        program = &_directionalLightShadowMap;
        locations = &_directionalLightShadowMapLocations;
        if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
            program = &_directionalLightCascadedShadowMap;
            locations = &_directionalLightCascadedShadowMapLocations;
            _directionalLightCascadedShadowMap.bind();
            _directionalLightCascadedShadowMap.setUniform(locations->shadowDistances,
                Application::getInstance()->getShadowDistances());
        
        } else {
            program->bind();
        }
        program->setUniformValue(locations->shadowScale,
            1.0f / Application::getInstance()->getTextureCache()->getShadowFramebufferObject()->width());
        
    } else {
        program->bind();
    }
    
    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    Application::getInstance()->computeOffAxisFrustum(
        left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    program->setUniformValue(locations->nearLocation, nearVal);
    program->setUniformValue(locations->depthScale, (farVal - nearVal) / farVal);
    float nearScale = -1.0f / nearVal;
    float depthTexCoordScaleS = (right - left) * nearScale / sWidth;
    float depthTexCoordScaleT = (top - bottom) * nearScale / tHeight;
    program->setUniformValue(locations->depthTexCoordOffset, left * nearScale - sMin * depthTexCoordScaleS,
        bottom * nearScale - tMin * depthTexCoordScaleT);
    program->setUniformValue(locations->depthTexCoordScale, depthTexCoordScaleS, depthTexCoordScaleT);
    
    renderFullscreenQuad(sMin, sMin + sWidth, tMin, tMin + tHeight);
    
    program->release();
    
    if (shadowsEnabled) {
        glBindTexture(GL_TEXTURE_2D, 0);        
        glActiveTexture(GL_TEXTURE3);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
        
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    freeFBO->release();
    
    // now transfer the lit region to the primary fbo
    glEnable(GL_BLEND);
    
    primaryFBO->bind();
    
    glBindTexture(GL_TEXTURE_2D, freeFBO->texture());
    glEnable(GL_TEXTURE_2D);
    
    renderFullscreenQuad(sMin, sMin + sWidth, tMin, tMin + tHeight);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // now render the objects we held back until after deferred lighting
    foreach (PostLightingRenderable* renderable, _postLightingRenderables) {
        renderable->renderPostLighting();
    }
    _postLightingRenderables.clear();
}

void DeferredLightingEffect::loadLightProgram(const char* name, ProgramObject& program, LightLocations& locations) {
    program.addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + name);
    program.link();
    
    program.bind();
    program.setUniformValue("diffuseMap", 0);
    program.setUniformValue("normalMap", 1);
    program.setUniformValue("specularMap", 2);
    program.setUniformValue("depthMap", 3);
    program.setUniformValue("shadowMap", 4);
    locations.shadowDistances = program.uniformLocation("shadowDistances");
    locations.shadowScale = program.uniformLocation("shadowScale");
    locations.nearLocation = program.uniformLocation("near");
    locations.depthScale = program.uniformLocation("depthScale");
    locations.depthTexCoordOffset = program.uniformLocation("depthTexCoordOffset");
    locations.depthTexCoordScale = program.uniformLocation("depthTexCoordScale");
    program.release();
}
