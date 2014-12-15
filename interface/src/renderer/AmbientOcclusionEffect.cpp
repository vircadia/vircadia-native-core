//
//  AmbientOcclusionEffect.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 7/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include <glm/gtc/random.hpp>

#include <ProgramObject.h>
#include <SharedUtil.h>

#include "Application.h"
#include "RenderUtil.h"

#include "AmbientOcclusionEffect.h"

const int ROTATION_WIDTH = 4;
const int ROTATION_HEIGHT = 4;
    
void AmbientOcclusionEffect::init() {
    
    _occlusionProgram = new ProgramObject();
    _occlusionProgram->addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath()
                                               + "shaders/ambient_occlusion.vert");
    _occlusionProgram->addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath()
                                               + "shaders/ambient_occlusion.frag");
    _occlusionProgram->link();
    
    // create the sample kernel: an array of hemispherically distributed offset vectors
    const int SAMPLE_KERNEL_SIZE = 16;
    QVector3D sampleKernel[SAMPLE_KERNEL_SIZE];
    for (int i = 0; i < SAMPLE_KERNEL_SIZE; i++) {
        // square the length in order to increase density towards the center
        glm::vec3 vector = glm::sphericalRand(1.0f);
        float scale = randFloat();
        const float MIN_VECTOR_LENGTH = 0.01f;
        const float MAX_VECTOR_LENGTH = 1.0f;
        vector *= glm::mix(MIN_VECTOR_LENGTH, MAX_VECTOR_LENGTH, scale * scale);
        sampleKernel[i] = QVector3D(vector.x, vector.y, vector.z);
    }
    
    _occlusionProgram->bind();
    _occlusionProgram->setUniformValue("depthTexture", 0);
    _occlusionProgram->setUniformValue("rotationTexture", 1);
    _occlusionProgram->setUniformValueArray("sampleKernel", sampleKernel, SAMPLE_KERNEL_SIZE);
    _occlusionProgram->setUniformValue("radius", 0.1f);
    _occlusionProgram->release();
    
    _nearLocation = _occlusionProgram->uniformLocation("near");
    _farLocation = _occlusionProgram->uniformLocation("far");
    _leftBottomLocation = _occlusionProgram->uniformLocation("leftBottom");
    _rightTopLocation = _occlusionProgram->uniformLocation("rightTop");
    _noiseScaleLocation = _occlusionProgram->uniformLocation("noiseScale");
    _texCoordOffsetLocation = _occlusionProgram->uniformLocation("texCoordOffset");
    _texCoordScaleLocation = _occlusionProgram->uniformLocation("texCoordScale");
    
    // generate the random rotation texture
    glGenTextures(1, &_rotationTextureID);
    glBindTexture(GL_TEXTURE_2D, _rotationTextureID);
    const int ELEMENTS_PER_PIXEL = 3;
    unsigned char rotationData[ROTATION_WIDTH * ROTATION_HEIGHT * ELEMENTS_PER_PIXEL];
    unsigned char* rotation = rotationData;
    for (int i = 0; i < ROTATION_WIDTH * ROTATION_HEIGHT; i++) {
        glm::vec3 vector = glm::sphericalRand(1.0f);
        *rotation++ = ((vector.x + 1.0f) / 2.0f) * 255.0f;
        *rotation++ = ((vector.y + 1.0f) / 2.0f) * 255.0f;
        *rotation++ = ((vector.z + 1.0f) / 2.0f) * 255.0f;
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ROTATION_WIDTH, ROTATION_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, rotationData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    _blurProgram = new ProgramObject();
    _blurProgram->addShaderFromSourceFile(QGLShader::Vertex, Application::resourcesPath() + "shaders/ambient_occlusion.vert");
    _blurProgram->addShaderFromSourceFile(QGLShader::Fragment, Application::resourcesPath() + "shaders/occlusion_blur.frag");
    _blurProgram->link();
    
    _blurProgram->bind();
    _blurProgram->setUniformValue("originalTexture", 0);
    _blurProgram->release();
    
    _blurScaleLocation = _blurProgram->uniformLocation("blurScale");
}

void AmbientOcclusionEffect::render() {
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glBindTexture(GL_TEXTURE_2D, DependencyManager::get<TextureCache>()->getPrimaryDepthTextureID());
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _rotationTextureID);
    
    // render with the occlusion shader to the secondary/tertiary buffer
    QOpenGLFramebufferObject* freeFBO = Application::getInstance()->getGlowEffect()->getFreeFramebufferObject();
    freeFBO->bind();
    
    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    Application::getInstance()->computeOffAxisFrustum(
        left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_X_INDEX = 0;
    const int VIEWPORT_WIDTH_INDEX = 2;
    QOpenGLFramebufferObject* primaryFBO = DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject();
    float sMin = viewport[VIEWPORT_X_INDEX] / (float)primaryFBO->width();
    float sWidth = viewport[VIEWPORT_WIDTH_INDEX] / (float)primaryFBO->width();
    
    _occlusionProgram->bind();
    _occlusionProgram->setUniformValue(_nearLocation, nearVal);
    _occlusionProgram->setUniformValue(_farLocation, farVal);
    _occlusionProgram->setUniformValue(_leftBottomLocation, left, bottom);
    _occlusionProgram->setUniformValue(_rightTopLocation, right, top);
    _occlusionProgram->setUniformValue(_noiseScaleLocation, viewport[VIEWPORT_WIDTH_INDEX] / (float)ROTATION_WIDTH,
        primaryFBO->height() / (float)ROTATION_HEIGHT);
    _occlusionProgram->setUniformValue(_texCoordOffsetLocation, sMin, 0.0f);
    _occlusionProgram->setUniformValue(_texCoordScaleLocation, sWidth, 1.0f);
    
    renderFullscreenQuad();
    
    _occlusionProgram->release();
    
    freeFBO->release();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE0);
    
    // now render secondary to primary with 4x4 blur
    DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->bind();
    
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
    
    glBindTexture(GL_TEXTURE_2D, freeFBO->texture());
    
    _blurProgram->bind();
    _blurProgram->setUniformValue(_blurScaleLocation, 1.0f / primaryFBO->width(), 1.0f / primaryFBO->height());
    
    renderFullscreenQuad(sMin, sMin + sWidth);
    
    _blurProgram->release();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}
