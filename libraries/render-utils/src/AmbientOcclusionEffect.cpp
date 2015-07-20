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

#include "AmbientOcclusionEffect.h"

#include <QFile>
#include <QTextStream>

#include <gpu/GLBackend.h>
#include <glm/gtc/random.hpp>
#include <PathUtils.h>
#include <SharedUtil.h>
#include <ViewFrustum.h>

#include "AbstractViewStateInterface.h"
#include "RenderUtil.h"
#include "TextureCache.h"

const int ROTATION_WIDTH = 4;
const int ROTATION_HEIGHT = 4;
    
static QString readFile(const QString& path) {
    QFile file(path);
    QString result;
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&file);
        result = in.readAll();
    }
    return result;
}

void AmbientOcclusionEffect::init(AbstractViewStateInterface* viewState) {
#if 0
    _viewState = viewState; // we will use this for view state services

    {
        gpu::Shader::Source vertexSource = std::string(readFile(PathUtils::resourcesPath()
            + "shaders/ambient_occlusion.vert").toLocal8Bit().data());
        gpu::Shader::Source fragmentSource = std::string(readFile(PathUtils::resourcesPath()
            + "shaders/ambient_occlusion.frag").toLocal8Bit().data());
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(vertexSource));
        auto fs = gpu::ShaderPointer(gpu::Shader::createPixel(fragmentSource));
        _occlusionProgram = gpu::ShaderPointer(gpu::Shader::createProgram(vs, fs));
    }
    foreach(auto uniform, _occlusionProgram->getUniforms()) {
        qDebug() << uniform._location;
    }

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
    
    glUseProgram(gpu::GLBackend::getShaderID(_occlusionProgram));
    glUniform1i(_occlusionProgram->getUniforms().findLocation("depthTexture"), 0);
    glUniform1i(_occlusionProgram->getUniforms().findLocation("rotationTexture"), 1);
    glUniform1f(_occlusionProgram->getUniforms().findLocation("radius"), 0.1f);
    //_occlusionProgram->setUniformValueArray("sampleKernel", sampleKernel, SAMPLE_KERNEL_SIZE);
    glUseProgram(0);

    _nearLocation = _occlusionProgram->getUniforms().findLocation("near");
    _farLocation = _occlusionProgram->getUniforms().findLocation("far");
    _leftBottomLocation = _occlusionProgram->getUniforms().findLocation("leftBottom");
    _rightTopLocation = _occlusionProgram->getUniforms().findLocation("rightTop");
    _noiseScaleLocation = _occlusionProgram->getUniforms().findLocation("noiseScale");
    _texCoordOffsetLocation = _occlusionProgram->getUniforms().findLocation("texCoordOffset");
    _texCoordScaleLocation = _occlusionProgram->getUniforms().findLocation("texCoordScale");
    
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

    {
        gpu::Shader::Source vertexSource = std::string(readFile(PathUtils::resourcesPath()
            + "shaders/ambient_occlusion.vert").toLocal8Bit().data());
        gpu::Shader::Source fragmentSource = std::string(readFile(PathUtils::resourcesPath()
            + "shaders/occlusion_blur.frag").toLocal8Bit().data());
        auto vs = gpu::ShaderPointer(gpu::Shader::createVertex(vertexSource));
        auto fs = gpu::ShaderPointer(gpu::Shader::createPixel(fragmentSource));
        _blurProgram = gpu::ShaderPointer(gpu::Shader::createProgram(vs, fs));
    }

    glUseProgram(gpu::GLBackend::getShaderID(_blurProgram));
    foreach(auto uniform, _blurProgram->getUniforms()) {
        qDebug() << uniform._location;
    }
    glUniform1f(_blurProgram->getUniforms().findLocation("originalTexture"), 0);
    glUseProgram(0);
    _blurScaleLocation = _blurProgram->getUniforms().findLocation("blurScale");
#endif
}

void AmbientOcclusionEffect::render() {
#if 0
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glBindTexture(GL_TEXTURE_2D, DependencyManager::get<TextureCache>()->getPrimaryDepthTextureID());
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _rotationTextureID);
    
    // render with the occlusion shader to the secondary/tertiary buffer
    auto freeFramebuffer = nullptr; // DependencyManager::get<GlowEffect>()->getFreeFramebuffer(); // FIXME
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(freeFramebuffer));
    
    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    auto viewFrustum = _viewState->getCurrentViewFrustum();
    viewFrustum->computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int VIEWPORT_X_INDEX = 0;
    const int VIEWPORT_WIDTH_INDEX = 2;

    auto framebufferSize = DependencyManager::get<TextureCache>()->getFrameBufferSize();
    float sMin = viewport[VIEWPORT_X_INDEX] / (float)framebufferSize.width();
    float sWidth = viewport[VIEWPORT_WIDTH_INDEX] / (float)framebufferSize.width();
    
    glUseProgram(gpu::GLBackend::getShaderID(_occlusionProgram));
    glUniform1f(_nearLocation, nearVal);
    glUniform1f(_farLocation, farVal);
    glUniform2f(_leftBottomLocation, left, bottom);
    glUniform2f(_rightTopLocation, right, top);
    glUniform2f(_noiseScaleLocation, viewport[VIEWPORT_WIDTH_INDEX] / (float)ROTATION_WIDTH,
        framebufferSize.height() / (float)ROTATION_HEIGHT);
    glUniform2f(_texCoordOffsetLocation, sMin, 0.0f);
    glUniform2f(_texCoordScaleLocation, sWidth, 1.0f);
    
    renderFullscreenQuad();
    
    glUseProgram(0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE0);
    
    // now render secondary to primary with 4x4 blur
    auto primaryFramebuffer = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(primaryFramebuffer));

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
    
    auto freeFramebufferTexture = nullptr; // freeFramebuffer->getRenderBuffer(0); // FIXME
    glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(freeFramebufferTexture));
    
    glUseProgram(gpu::GLBackend::getShaderID(_occlusionProgram));
    glUniform2f(_blurScaleLocation, 1.0f / framebufferSize.width(), 1.0f / framebufferSize.height());
    
    renderFullscreenQuad(sMin, sMin + sWidth);
    
    glUseProgram(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
#endif
}
