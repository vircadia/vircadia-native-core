//
//  GlowEffect.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include "Application.h"
#include "GlowEffect.h"
#include "ProgramObject.h"
#include "RenderUtil.h"

GlowEffect::GlowEffect() : _renderMode(BLUR_ADD_MODE) {
}

static ProgramObject* createProgram(const QString& name) {
    ProgramObject* program = new ProgramObject();
    program->addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/" + name + ".frag");
    program->link();
    
    program->bind();
    program->setUniformValue("originalTexture", 0);
    program->release();
    
    return program;
}

void GlowEffect::init() {
    switchToResourcesParentIfRequired();
    
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
}

void GlowEffect::prepare() {
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    _isEmpty = true;
}

void GlowEffect::begin(float intensity) {
    glBlendColor(0.0f, 0.0f, 0.0f, intensity);
    _isEmpty = false;
}

void GlowEffect::end() {
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void GlowEffect::render() {
    QOpenGLFramebufferObject* primaryFBO = Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject();
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
    
    if (_isEmpty) {
        // copy the primary to the screen
        if (QOpenGLFramebufferObject::hasOpenGLFramebufferBlit()) {
            QOpenGLFramebufferObject::blitFramebuffer(NULL, primaryFBO);
                
        } else {
            glEnable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glColor3f(1.0f, 1.0f, 1.0f);
            renderFullscreenQuad();    
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
        }
    } else if (_renderMode == ADD_MODE) {
        _addProgram->bind();
        renderFullscreenQuad();
        _addProgram->release();
    
    } else if (_renderMode == DIFFUSE_ADD_MODE) {
        // diffuse into the secondary/tertiary (alternating between frames)
        QOpenGLFramebufferObject* oldDiffusedFBO =
            Application::getInstance()->getTextureCache()->getSecondaryFramebufferObject();
        QOpenGLFramebufferObject* newDiffusedFBO =
            Application::getInstance()->getTextureCache()->getTertiaryFramebufferObject();
        if ((_isOddFrame = !_isOddFrame)) {
            qSwap(oldDiffusedFBO, newDiffusedFBO);
        }
        newDiffusedFBO->bind();
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, oldDiffusedFBO->texture());
            
        _diffuseProgram->bind();
        renderFullscreenQuad();
        _diffuseProgram->release();
        
        newDiffusedFBO->release();
        
        // add diffused texture to the primary
        glBindTexture(GL_TEXTURE_2D, newDiffusedFBO->texture());
        
        _addSeparateProgram->bind();      
        renderFullscreenQuad();
        _addSeparateProgram->release();
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        
    } else { // _renderMode == BLUR_ADD_MODE || _renderMode == BLUR_PERSIST_ADD_MODE
        // render the primary to the secondary with the horizontal blur
        QOpenGLFramebufferObject* secondaryFBO =
            Application::getInstance()->getTextureCache()->getSecondaryFramebufferObject();
        secondaryFBO->bind();
     
        _horizontalBlurProgram->bind();
        renderFullscreenQuad();
        _horizontalBlurProgram->release();
     
        secondaryFBO->release();
        
        if (_renderMode == BLUR_ADD_MODE) {
            // render the secondary to the screen with the vertical blur
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, secondaryFBO->texture());
            
            _verticalBlurAddProgram->bind();      
            renderFullscreenQuad();
            _verticalBlurAddProgram->release();
            
        } else { // _renderMode == BLUR_PERSIST_ADD_MODE
            // render the secondary to the tertiary with horizontal blur and persistence
            QOpenGLFramebufferObject* tertiaryFBO =
                Application::getInstance()->getTextureCache()->getTertiaryFramebufferObject();
            tertiaryFBO->bind();
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE_MINUS_CONSTANT_ALPHA, GL_CONSTANT_ALPHA);
            const float PERSISTENCE_SMOOTHING = 0.9f;
            glBlendColor(0.0f, 0.0f, 0.0f, PERSISTENCE_SMOOTHING);
            
            glBindTexture(GL_TEXTURE_2D, secondaryFBO->texture());
            
            _verticalBlurProgram->bind();      
            renderFullscreenQuad();
            _verticalBlurProgram->release();
        
            glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
            glDisable(GL_BLEND);
        
            // now add the tertiary to the primary buffer
            tertiaryFBO->release();
            
            glBindTexture(GL_TEXTURE_2D, primaryFBO->texture());
            
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, tertiaryFBO->texture());
            
            _addSeparateProgram->bind();      
            renderFullscreenQuad();
            _addSeparateProgram->release();
        }
        
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
}

void GlowEffect::cycleRenderMode() {
    _renderMode = (RenderMode)((_renderMode + 1) % RENDER_MODE_COUNT);
}
