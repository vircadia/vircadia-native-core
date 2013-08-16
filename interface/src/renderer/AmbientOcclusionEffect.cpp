//
//  AmbientOcclusionEffect.cpp
//  interface
//
//  Created by Andrzej Kapolka on 7/14/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <SharedUtil.h>

#include "AmbientOcclusionEffect.h"
#include "Application.h"
#include "InterfaceConfig.h"
#include "ProgramObject.h"
#include "RenderUtil.h"

void AmbientOcclusionEffect::init() {
    switchToResourcesParentIfRequired();
    
    _program = new ProgramObject();
    _program->addShaderFromSourceFile(QGLShader::Fragment, "resources/shaders/ambient_occlusion.frag");
    _program->link();
    
    _nearLocation = _program->uniformLocation("near");
    _farLocation = _program->uniformLocation("far");
    _leftBottomLocation = _program->uniformLocation("leftBottom");
    _rightTopLocation = _program->uniformLocation("rightTop");
    
    _program->bind();
    _program->setUniformValue("depthTexture", 0);
    _program->release();
}

void AmbientOcclusionEffect::render() {
    glPushMatrix();
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_BLEND);
    //glBlendFuncSeparate(GL_ZERO, GL_SRC_COLOR, GL_ZERO, GL_ONE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glBindTexture(GL_TEXTURE_2D, Application::getInstance()->getTextureCache()->getPrimaryDepthTextureID());
    
    float left, right, bottom, top, nearVal, farVal;
    glm::vec4 nearClipPlane, farClipPlane;
    Application::getInstance()->getViewFrustum()->computeOffAxisFrustum(
        left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    
    _program->bind();
    _program->setUniformValue(_nearLocation, nearVal);
    _program->setUniformValue(_farLocation, farVal);
    _program->setUniformValue(_leftBottomLocation, left, bottom);
    _program->setUniformValue(_rightTopLocation, right, top);
    
    renderFullscreenQuad();
    
    _program->release();
    
    glPopMatrix();
    
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
       
    glEnable(GL_BLEND);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glBindTexture(GL_TEXTURE_2D, 0);
}
