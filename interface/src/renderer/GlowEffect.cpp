//
//  GlowEffect.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include "Application.h"
#include "GlowEffect.h"
#include "ProgramObject.h"

static ProgramObject* createBlurProgram(const QString& direction) {
    ProgramObject* program = new ProgramObject();
    program->addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/" + direction + "_blur.frag");
    program->link();
    
    program->bind();
    program->setUniformValue("originalTexture", 0);
    program->release();
    
    return program;
}

void GlowEffect::init() {
    switchToResourcesParentIfRequired();
    _horizontalBlurProgram = createBlurProgram("horizontal");
    _verticalBlurProgram = createBlurProgram("vertical");
    
    _verticalBlurProgram->bind();
    _verticalBlurProgram->setUniformValue("horizontallyBlurredTexture", 1);
    _verticalBlurProgram->release();
}

void GlowEffect::prepare() {
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    _isEmpty = true;
}

void GlowEffect::begin(float amount) {
    glBlendColor(0.0f, 0.0f, 0.0f, amount);
    _isEmpty = false;
}

void GlowEffect::end() {
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);
}

static void renderFullscreenQuad() {
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
    glEnd();
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
    } else {
        // render the primary to the secondary with the horizontal blur
        QOpenGLFramebufferObject* secondaryFBO =
            Application::getInstance()->getTextureCache()->getSecondaryFramebufferObject();
        secondaryFBO->bind();
     
        _horizontalBlurProgram->bind();
        renderFullscreenQuad();
        _horizontalBlurProgram->release();
     
        secondaryFBO->release();
        
        // render the secondary to the screen with the vertical blur
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, secondaryFBO->texture());
        
        _verticalBlurProgram->bind();      
        renderFullscreenQuad();
        _verticalBlurProgram->release();
        
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
       
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
}
