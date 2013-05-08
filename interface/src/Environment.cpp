//
//  Environment.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "Camera.h"
#include "Environment.h"
#include "renderer/ProgramObject.h"
#include "renderer/ShaderObject.h"
#include "world.h"

void Environment::init() {
    _skyFromAtmosphereProgram = new ProgramObject();
    _skyFromAtmosphereProgram->attachFromSourceFile(GL_VERTEX_SHADER_ARB, "resources/shaders/SkyFromAtmosphere.vert");
    _skyFromAtmosphereProgram->attachFromSourceFile(GL_FRAGMENT_SHADER_ARB, "resources/shaders/SkyFromAtmosphere.frag");
    _skyFromAtmosphereProgram->link();

    _cameraPosLocation = _skyFromAtmosphereProgram->getUniformLocation("v3CameraPos");
    _lightPosLocation = _skyFromAtmosphereProgram->getUniformLocation("v3LightPos");
    _invWavelengthLocation = _skyFromAtmosphereProgram->getUniformLocation("v3InvWavelength");
    _innerRadiusLocation = _skyFromAtmosphereProgram->getUniformLocation("fInnerRadius");
    _krESunLocation = _skyFromAtmosphereProgram->getUniformLocation("fKrESun");
    _kmESunLocation = _skyFromAtmosphereProgram->getUniformLocation("fKmESun");
    _kr4PiLocation = _skyFromAtmosphereProgram->getUniformLocation("fKr4PI");
    _km4PiLocation = _skyFromAtmosphereProgram->getUniformLocation("fKm4PI");
    _scaleLocation = _skyFromAtmosphereProgram->getUniformLocation("fScale");
    _scaleDepthLocation = _skyFromAtmosphereProgram->getUniformLocation("fScaleDepth");
    _scaleOverScaleDepthLocation = _skyFromAtmosphereProgram->getUniformLocation("fScaleOverScaleDepth");
    _gLocation = _skyFromAtmosphereProgram->getUniformLocation("g");
    _g2Location = _skyFromAtmosphereProgram->getUniformLocation("g2");
}

void Environment::render(Camera& camera) {    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(getAtmosphereCenter().x, getAtmosphereCenter().y, getAtmosphereCenter().z);

    // use the camera distance to reset the near and far distances to keep the atmosphere in the frustum
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    float projection[16];
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glm::vec3 relativeCameraPos = camera.getPosition() - getAtmosphereCenter();
    float near = camera.getNearClip(), far = glm::length(relativeCameraPos) + getAtmosphereOuterRadius();
    projection[10] = (far + near) / (near - far);
    projection[14] = (2.0f * far * near) / (near - far);
    glLoadMatrixf(projection);
    
    // the constants here are from Sean O'Neil's GPU Gems entry
    // (http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html), GameEngine.cpp
    _skyFromAtmosphereProgram->bind();
    _skyFromAtmosphereProgram->setUniform(_cameraPosLocation, relativeCameraPos);
    glm::vec3 lightDirection = glm::normalize(getSunLocation());
    _skyFromAtmosphereProgram->setUniform(_lightPosLocation, lightDirection);
    _skyFromAtmosphereProgram->setUniform(_invWavelengthLocation,
        1 / powf(getScatteringWavelengths().r, 4.0f),
        1 / powf(getScatteringWavelengths().g, 4.0f),
        1 / powf(getScatteringWavelengths().b, 4.0f));
    _skyFromAtmosphereProgram->setUniform(_innerRadiusLocation, getAtmosphereInnerRadius());
    _skyFromAtmosphereProgram->setUniform(_krESunLocation, getRayleighScattering() * getSunBrightness());
    _skyFromAtmosphereProgram->setUniform(_kmESunLocation, getMieScattering() * getSunBrightness());
    _skyFromAtmosphereProgram->setUniform(_kr4PiLocation, getRayleighScattering() * 4.0f * PIf);
    _skyFromAtmosphereProgram->setUniform(_km4PiLocation, getMieScattering() * 4.0f * PIf);
    _skyFromAtmosphereProgram->setUniform(_scaleLocation, 1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius()));
    _skyFromAtmosphereProgram->setUniform(_scaleDepthLocation, 0.25f);
    _skyFromAtmosphereProgram->setUniform(_scaleOverScaleDepthLocation,
        (1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius())) / 0.25f);
    _skyFromAtmosphereProgram->setUniform(_gLocation, -0.990f);
    _skyFromAtmosphereProgram->setUniform(_g2Location, -0.990f * -0.990f);
    
    glFrontFace(GL_CW);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glutSolidSphere(getAtmosphereOuterRadius(), 100, 50);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    
    _skyFromAtmosphereProgram->release();
    
    glPopMatrix();
       
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();   
}
