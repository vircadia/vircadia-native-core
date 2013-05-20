//
//  Environment.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QByteArray>

#include <SharedUtil.h>

#include "Camera.h"
#include "Environment.h"
#include "renderer/ProgramObject.h"
#include "world.h"

void Environment::init() {
    switchToResourcesParentIfRequired();
    _skyFromAtmosphereProgram = createSkyProgram("Atmosphere", _skyFromAtmosphereUniformLocations);
    _skyFromSpaceProgram = createSkyProgram("Space", _skyFromSpaceUniformLocations);
}

void Environment::renderAtmosphere(Camera& camera) {    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(getAtmosphereCenter().x, getAtmosphereCenter().y, getAtmosphereCenter().z);

    glm::vec3 relativeCameraPos = camera.getPosition() - getAtmosphereCenter();
    float height = glm::length(relativeCameraPos);
    
    // use the appropriate shader depending on whether we're inside or outside
    ProgramObject* program;
    int* locations;
    if (height < getAtmosphereOuterRadius()) {
        program = _skyFromAtmosphereProgram;
        locations = _skyFromAtmosphereUniformLocations;
        
    } else {
        program = _skyFromSpaceProgram;
        locations = _skyFromSpaceUniformLocations;
    }
    
    // the constants here are from Sean O'Neil's GPU Gems entry
    // (http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter16.html), GameEngine.cpp
    program->bind();
    program->setUniform(locations[CAMERA_POS_LOCATION], relativeCameraPos);
    glm::vec3 lightDirection = glm::normalize(getSunLocation());
    program->setUniform(locations[LIGHT_POS_LOCATION], lightDirection);
    program->setUniformValue(locations[INV_WAVELENGTH_LOCATION],
        1 / powf(getScatteringWavelengths().r, 4.0f),
        1 / powf(getScatteringWavelengths().g, 4.0f),
        1 / powf(getScatteringWavelengths().b, 4.0f));
    program->setUniformValue(locations[CAMERA_HEIGHT2_LOCATION], height * height);
    program->setUniformValue(locations[OUTER_RADIUS_LOCATION], getAtmosphereOuterRadius());
    program->setUniformValue(locations[OUTER_RADIUS2_LOCATION], getAtmosphereOuterRadius() * getAtmosphereOuterRadius());
    program->setUniformValue(locations[INNER_RADIUS_LOCATION], getAtmosphereInnerRadius());
    program->setUniformValue(locations[KR_ESUN_LOCATION], getRayleighScattering() * getSunBrightness());
    program->setUniformValue(locations[KM_ESUN_LOCATION], getMieScattering() * getSunBrightness());
    program->setUniformValue(locations[KR_4PI_LOCATION], getRayleighScattering() * 4.0f * PIf);
    program->setUniformValue(locations[KM_4PI_LOCATION], getMieScattering() * 4.0f * PIf);
    program->setUniformValue(locations[SCALE_LOCATION], 1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius()));
    program->setUniformValue(locations[SCALE_DEPTH_LOCATION], 0.25f);
    program->setUniformValue(locations[SCALE_OVER_SCALE_DEPTH_LOCATION],
        (1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius())) / 0.25f);
    program->setUniformValue(locations[G_LOCATION], -0.990f);
    program->setUniformValue(locations[G2_LOCATION], -0.990f * -0.990f);
    
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glutSolidSphere(getAtmosphereOuterRadius(), 100, 50);
    glDepthMask(GL_TRUE);
    
    program->release();
    
    glPopMatrix();
       
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();   
}

ProgramObject* Environment::createSkyProgram(const char* from, int* locations) {
    ProgramObject* program = new ProgramObject();
    QByteArray prefix = QByteArray("resources/shaders/SkyFrom") + from;
    program->addShaderFromSourceFile(QGLShader::Vertex, prefix + ".vert");
    program->addShaderFromSourceFile(QGLShader::Fragment, prefix + ".frag");
    program->link();
    
    locations[CAMERA_POS_LOCATION] = program->uniformLocation("v3CameraPos");
    locations[LIGHT_POS_LOCATION] = program->uniformLocation("v3LightPos");
    locations[INV_WAVELENGTH_LOCATION] = program->uniformLocation("v3InvWavelength");
    locations[CAMERA_HEIGHT2_LOCATION] = program->uniformLocation("fCameraHeight2");
    locations[OUTER_RADIUS_LOCATION] = program->uniformLocation("fOuterRadius");
    locations[OUTER_RADIUS2_LOCATION] = program->uniformLocation("fOuterRadius2");
    locations[INNER_RADIUS_LOCATION] = program->uniformLocation("fInnerRadius");
    locations[KR_ESUN_LOCATION] = program->uniformLocation("fKrESun");
    locations[KM_ESUN_LOCATION] = program->uniformLocation("fKmESun");
    locations[KR_4PI_LOCATION] = program->uniformLocation("fKr4PI");
    locations[KM_4PI_LOCATION] = program->uniformLocation("fKm4PI");
    locations[SCALE_LOCATION] = program->uniformLocation("fScale");
    locations[SCALE_DEPTH_LOCATION] = program->uniformLocation("fScaleDepth");
    locations[SCALE_OVER_SCALE_DEPTH_LOCATION] = program->uniformLocation("fScaleOverScaleDepth");
    locations[G_LOCATION] = program->uniformLocation("g");
    locations[G2_LOCATION] = program->uniformLocation("g2");
    
    return program;
}
