//
//  GlowEffect.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include <gpu/GPUConfig.h>

#include <QOpenGLFramebufferObject>
#include <QWindow>

#include <PathUtils.h>
#include <PerfStat.h>

#include "GlowEffect.h"
#include "ProgramObject.h"
#include "RenderUtil.h"
#include "TextureCache.h"
#include "RenderUtilsLogging.h"

#include "gpu/GLBackend.h"

GlowEffect::GlowEffect()
    : _initialized(false),
      _isOddFrame(false),
      _isFirstFrame(true),
      _intensity(0.0f),
      _enabled(false) {
}

GlowEffect::~GlowEffect() {
    if (_initialized) {
        delete _addProgram;
        delete _horizontalBlurProgram;
        delete _verticalBlurAddProgram;
        delete _verticalBlurProgram;
        delete _addSeparateProgram;
        delete _diffuseProgram;
    }
}

gpu::FramebufferPointer GlowEffect::getFreeFramebuffer() const {
    return (_isOddFrame ?
                DependencyManager::get<TextureCache>()->getSecondaryFramebuffer():
                DependencyManager::get<TextureCache>()->getTertiaryFramebuffer());
}

static ProgramObject* createProgram(const QString& name) {
    ProgramObject* program = new ProgramObject();
    program->addShaderFromSourceFile(QGLShader::Fragment, PathUtils::resourcesPath() + "shaders/" + name + ".frag");
    program->link();
    
    program->bind();
    program->setUniformValue("originalTexture", 0);
    program->release();
    
    return program;
}

void GlowEffect::init(bool enabled) {
    if (_initialized) {
        qCDebug(renderutils, "[ERROR] GlowEffeect is already initialized.");
        return;
    }
    
    _addProgram = createProgram("glow_add");
    _horizontalBlurProgram = createProgram("horizontal_blur");
    _verticalBlurAddProgram = createProgram("vertical_blur_add");
    _verticalBlurProgram = createProgram("vertical_blur");
    _addSeparateProgram = createProgram("glow_add_separate");
    _diffuseProgram = createProgram("diffuse");
    
    _verticalBlurAddProgram->bind();
    _verticalBlurAddProgram->setUniformValue("horizontallyBlurredTexture", 1);
    _verticalBlurAddProgram->release();
    
    _addSeparateProgram->bind();
    _addSeparateProgram->setUniformValue("blurredTexture", 1);
    _addSeparateProgram->release();
    
    _diffuseProgram->bind();
    _diffuseProgram->setUniformValue("diffusedTexture", 1);
    _diffuseProgram->release();
    
    _diffusionScaleLocation = _diffuseProgram->uniformLocation("diffusionScale");

    _initialized = true;
    _enabled = enabled;
}

void GlowEffect::prepare() {
    auto primaryFBO = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer();
    GLuint fbo = gpu::GLBackend::getFramebufferID(primaryFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    _isEmpty = true;
    _isOddFrame = !_isOddFrame;
}

void GlowEffect::begin(float intensity) {
    // store the current intensity and add the new amount
    _intensityStack.push(_intensity);
    glBlendColor(0.0f, 0.0f, 0.0f, _intensity += intensity);
    _isEmpty &= (_intensity == 0.0f);
}

void GlowEffect::end() {
    // restore the saved intensity
    glBlendColor(0.0f, 0.0f, 0.0f, _intensity = _intensityStack.pop());
}

static void maybeBind(const gpu::FramebufferPointer& fbo) {
    if (fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(fbo));
    }
}

static void maybeRelease(const gpu::FramebufferPointer& fbo) {
    if (fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

gpu::FramebufferPointer GlowEffect::render(bool toTexture) {
    PerformanceTimer perfTimer("glowEffect");

    auto textureCache = DependencyManager::get<TextureCache>();

    auto primaryFBO = gpu::GLBackend::getFramebufferID(textureCache->getPrimaryFramebuffer());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, textureCache->getPrimaryColorTextureID());
    auto framebufferSize = textureCache->getFrameBufferSize();

    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    gpu::FramebufferPointer destFBO = toTexture ?
        textureCache->getSecondaryFramebuffer() : nullptr;
    if (!_enabled || _isEmpty) {
        // copy the primary to the screen
        if (destFBO && QOpenGLFramebufferObject::hasOpenGLFramebufferBlit()) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, primaryFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(destFBO));
            glBlitFramebuffer(0, 0, framebufferSize.width(), framebufferSize.height(), 0, 0, framebufferSize.width(), framebufferSize.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
        } else {
            maybeBind(destFBO);
            if (!destFBO) {
                //destFBO->getSize();
                glViewport(0, 0, framebufferSize.width(), framebufferSize.height());
            }
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            renderFullscreenQuad();
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            maybeRelease(destFBO);
        }
    } else {
        // diffuse into the secondary/tertiary (alternating between frames)
        auto oldDiffusedFBO =
            textureCache->getSecondaryFramebuffer();
        auto newDiffusedFBO =
            textureCache->getTertiaryFramebuffer();
        if (_isOddFrame) {
            qSwap(oldDiffusedFBO, newDiffusedFBO);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(newDiffusedFBO));
        
        if (_isFirstFrame) {
            glClear(GL_COLOR_BUFFER_BIT);    
            
        } else {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(oldDiffusedFBO->getRenderBuffer(0)));
            
            _diffuseProgram->bind();

            _diffuseProgram->setUniformValue(_diffusionScaleLocation, 1.0f / framebufferSize.width(), 1.0f / framebufferSize.height());
        
            renderFullscreenQuad();
        
            _diffuseProgram->release();
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // add diffused texture to the primary
        glBindTexture(GL_TEXTURE_2D, gpu::GLBackend::getTextureID(newDiffusedFBO->getRenderBuffer(0)));
        
        if (toTexture) {
            destFBO = oldDiffusedFBO;
        }
        maybeBind(destFBO);
        if (!destFBO) {
            glViewport(0, 0, framebufferSize.width(), framebufferSize.height());
        }
        _addSeparateProgram->bind();
        renderFullscreenQuad();
        _addSeparateProgram->release();
        maybeRelease(destFBO);
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
       
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    _isFirstFrame = false;
    
    return destFBO;
}

void GlowEffect::toggleGlowEffect(bool enabled) {
    _enabled = enabled;
}

Glower::Glower(float amount) {
    DependencyManager::get<GlowEffect>()->begin(amount);
}

Glower::~Glower() {
    DependencyManager::get<GlowEffect>()->end();
}

