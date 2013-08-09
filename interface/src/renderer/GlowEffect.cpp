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
    program->setUniformValue("colorTexture", 0);
    program->release();
    
    return program;
}

void GlowEffect::init() {
    switchToResourcesParentIfRequired();
    _horizontalBlurProgram = createBlurProgram("horizontal");
    _verticalBlurProgram = createBlurProgram("vertical");
}

void GlowEffect::prepare() {
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
    if (_isEmpty) {
        return; // nothing glowing
    }
    QOpenGLFramebufferObject* primaryFBO = Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject();
    glBindTexture(GL_TEXTURE_2D, primaryFBO->texture());

    QSize size = primaryFBO->size();
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, size.width(), size.height());

    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_BLEND);
    
    // render the primary to the secondary with the horizontal blur
    QOpenGLFramebufferObject* secondaryFBO = Application::getInstance()->getTextureCache()->getSecondaryFramebufferObject();
    secondaryFBO->bind();
 
    _horizontalBlurProgram->bind();
    renderFullscreenQuad();
    _horizontalBlurProgram->release();
 
    secondaryFBO->release();
    
    // render the secondary to the screen with the vertical blur
    glBindTexture(GL_TEXTURE_2D, secondaryFBO->texture());
    
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_CONSTANT_ALPHA, GL_ONE);
    
    _verticalBlurProgram->bind();      
    renderFullscreenQuad();
    _verticalBlurProgram->release();
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
}
