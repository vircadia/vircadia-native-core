//
//  GlowEffect.cpp
//  interface
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QOpenGLFramebufferObject>

#include "Application.h"
#include "GlowEffect.h"
#include "InterfaceConfig.h"
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
    bind();
    glClear(GL_COLOR_BUFFER_BIT);
    release();
}

void GlowEffect::bind() {
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->bind();
}

void GlowEffect::release() {
    Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->release();
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
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPrimaryFramebufferObject()->texture());
    glDisable(GL_LIGHTING);
    
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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    _verticalBlurProgram->bind();      
    renderFullscreenQuad();
    _verticalBlurProgram->release();
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
