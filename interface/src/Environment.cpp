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
#include "renderer/ShaderObject.h"
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

    // use the camera distance to reset the near and far distances to keep the atmosphere in the frustum
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    
    float projection[16];
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glm::vec3 relativeCameraPos = camera.getPosition() - getAtmosphereCenter();
    float height = glm::length(relativeCameraPos);
    float near = camera.getNearClip(), far = height + getAtmosphereOuterRadius();
    projection[10] = (far + near) / (near - far);
    projection[14] = (2.0f * far * near) / (near - far);
    glLoadMatrixf(projection);
    
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
    program->setUniform(locations[INV_WAVELENGTH_LOCATION],
        1 / powf(getScatteringWavelengths().r, 4.0f),
        1 / powf(getScatteringWavelengths().g, 4.0f),
        1 / powf(getScatteringWavelengths().b, 4.0f));
    program->setUniform(locations[CAMERA_HEIGHT2_LOCATION], height * height);
    program->setUniform(locations[OUTER_RADIUS_LOCATION], getAtmosphereOuterRadius());
    program->setUniform(locations[OUTER_RADIUS2_LOCATION], getAtmosphereOuterRadius() * getAtmosphereOuterRadius());
    program->setUniform(locations[INNER_RADIUS_LOCATION], getAtmosphereInnerRadius());
    program->setUniform(locations[KR_ESUN_LOCATION], getRayleighScattering() * getSunBrightness());
    program->setUniform(locations[KM_ESUN_LOCATION], getMieScattering() * getSunBrightness());
    program->setUniform(locations[KR_4PI_LOCATION], getRayleighScattering() * 4.0f * PIf);
    program->setUniform(locations[KM_4PI_LOCATION], getMieScattering() * 4.0f * PIf);
    program->setUniform(locations[SCALE_LOCATION], 1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius()));
    program->setUniform(locations[SCALE_DEPTH_LOCATION], 0.25f);
    program->setUniform(locations[SCALE_OVER_SCALE_DEPTH_LOCATION],
        (1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius())) / 0.25f);
    program->setUniform(locations[G_LOCATION], -0.990f);
    program->setUniform(locations[G2_LOCATION], -0.990f * -0.990f);
    
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
    program->attachFromSourceFile(GL_VERTEX_SHADER_ARB, prefix + ".vert");
    program->attachFromSourceFile(GL_FRAGMENT_SHADER_ARB, prefix + ".frag");
    program->link();
    
    locations[CAMERA_POS_LOCATION] = program->getUniformLocation("v3CameraPos");
    locations[LIGHT_POS_LOCATION] = program->getUniformLocation("v3LightPos");
    locations[INV_WAVELENGTH_LOCATION] = program->getUniformLocation("v3InvWavelength");
    locations[CAMERA_HEIGHT2_LOCATION] = program->getUniformLocation("fCameraHeight2");
    locations[OUTER_RADIUS_LOCATION] = program->getUniformLocation("fOuterRadius");
    locations[OUTER_RADIUS2_LOCATION] = program->getUniformLocation("fOuterRadius2");
    locations[INNER_RADIUS_LOCATION] = program->getUniformLocation("fInnerRadius");
    locations[KR_ESUN_LOCATION] = program->getUniformLocation("fKrESun");
    locations[KM_ESUN_LOCATION] = program->getUniformLocation("fKmESun");
    locations[KR_4PI_LOCATION] = program->getUniformLocation("fKr4PI");
    locations[KM_4PI_LOCATION] = program->getUniformLocation("fKm4PI");
    locations[SCALE_LOCATION] = program->getUniformLocation("fScale");
    locations[SCALE_DEPTH_LOCATION] = program->getUniformLocation("fScaleDepth");
    locations[SCALE_OVER_SCALE_DEPTH_LOCATION] = program->getUniformLocation("fScaleOverScaleDepth");
    locations[G_LOCATION] = program->getUniformLocation("g");
    locations[G2_LOCATION] = program->getUniformLocation("g2");
    
    return program;
}
