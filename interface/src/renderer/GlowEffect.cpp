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
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include <PathUtils.h>
#include <PerfStat.h>
#include <ProgramObject.h>

#include "Application.h"
#include "GlowEffect.h"
#include "RenderUtil.h"

GlowEffect::GlowEffect()
    : _initialized(false),
      _isOddFrame(false),
      _isFirstFrame(true),
      _intensity(0.0f) {
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

QOpenGLFramebufferObject* GlowEffect::getFreeFramebufferObject() const {
    return (_isOddFrame ?
                DependencyManager::get<TextureCache>()->getSecondaryFramebufferObject():
                DependencyManager::get<TextureCache>()->getTertiaryFramebufferObject());
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

void GlowEffect::init() {
    if (_initialized) {
        qDebug("[ERROR] GlowEffeect is already initialized.");
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
}

void GlowEffect::prepare() {
    DependencyManager::get<TextureCache>()->getPrimaryFramebufferObject()->bind();
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

static void maybeBind(QOpenGLFramebufferObject* fbo) {
    if (fbo) {
        fbo->bind();
    }
}

static void maybeRelease(QOpenGLFramebufferObject* fbo) {
    if (fbo) {
        fbo->release();
    }
}

QOpenGLFramebufferObject* GlowEffect::render(bool toTexture) {
    PerformanceTimer perfTimer("glowEffect");

    TextureCache* textureCache = DependencyManager::get<TextureCache>();
    QOpenGLFramebufferObject* primaryFBO = textureCache->getPrimaryFramebufferObject();
    primaryFBO->release();
    glBindTexture(GL_TEXTURE_2D, primaryFBO->texture());

    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    QOpenGLFramebufferObject* destFBO = toTexture ?
        textureCache->getSecondaryFramebufferObject() : NULL;
    if (!Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect) || _isEmpty) {
        // copy the primary to the screen
        if (destFBO && QOpenGLFramebufferObject::hasOpenGLFramebufferBlit()) {
            QOpenGLFramebufferObject::blitFramebuffer(destFBO, primaryFBO);          
        } else {
            maybeBind(destFBO);
            if (!destFBO) {
                glViewport(0, 0, Application::getInstance()->getGLWidget()->getDeviceWidth(),
                    Application::getInstance()->getGLWidget()->getDeviceHeight());
            }
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glColor3f(1.0f, 1.0f, 1.0f);
            renderFullscreenQuad();
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            maybeRelease(destFBO);
        }
    } else {
        // diffuse into the secondary/tertiary (alternating between frames)
        QOpenGLFramebufferObject* oldDiffusedFBO =
            textureCache->getSecondaryFramebufferObject();
        QOpenGLFramebufferObject* newDiffusedFBO =
            textureCache->getTertiaryFramebufferObject();
        if (_isOddFrame) {
            qSwap(oldDiffusedFBO, newDiffusedFBO);
        }
        newDiffusedFBO->bind();
        
        if (_isFirstFrame) {
            glClear(GL_COLOR_BUFFER_BIT);    
            
        } else {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, oldDiffusedFBO->texture());
            
            _diffuseProgram->bind();
            QSize size = primaryFBO->size();
            _diffuseProgram->setUniformValue(_diffusionScaleLocation, 1.0f / size.width(), 1.0f / size.height());
        
            renderFullscreenQuad();
        
            _diffuseProgram->release();
        }
        
        newDiffusedFBO->release();
        
        // add diffused texture to the primary
        glBindTexture(GL_TEXTURE_2D, newDiffusedFBO->texture());
        
        if (toTexture) {
            destFBO = oldDiffusedFBO;
        }
        maybeBind(destFBO);
        if (!destFBO) {
            glViewport(0, 0,
                Application::getInstance()->getGLWidget()->getDeviceWidth(),
                Application::getInstance()->getGLWidget()->getDeviceHeight());
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

Glower::Glower(float amount) {
    Application::getInstance()->getGlowEffect()->begin(amount);
}

Glower::~Glower() {
    Application::getInstance()->getGlowEffect()->end();
}

