//
//  AmbientOcclusionEffect.cpp
//  interface
//
//  Created by Andrzej Kapolka on 7/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <SharedUtil.h>

#include "AmbientOcclusionEffect.h"
#include "ProgramObject.h"

ProgramObject* AmbientOcclusionEffect::_program = 0;

AmbientOcclusionEffect::AmbientOcclusionEffect() : _depthTextureID(0) {
}

void AmbientOcclusionEffect::render(int screenWidth, int screenHeight) {
    // copy the z buffer into a depth texture
    if (_depthTextureID == 0) {
        glGenTextures(1, &_depthTextureID);
        glBindTexture(GL_TEXTURE_2D, _depthTextureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 0, 0, screenWidth, screenHeight, 0);
        
    } else {
        glBindTexture(GL_TEXTURE_2D, _depthTextureID);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, screenWidth, screenHeight);
    }
    
    // now render a full screen quad with that texture
    if (_program == 0) {
        switchToResourcesParentIfRequired();
        _program = new ProgramObject();
        _program->addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/ambient_occlusion.frag");
        _program->link();
        
        _program->bind();
        _program->setUniformValue("depthTexture", 0);
        
    } else {
        _program->bind();
    }
    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
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
     
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();   

    _program->release();
    glBindTexture(GL_TEXTURE_2D, 0);    
}
