//
//  Environment.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QFile>
#include <QtDebug>

#include "Camera.h"
#include "Environment.h"
#include "world.h"

static GLhandleARB compileShader(int type, const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't find file." << filename;
        return 0;
    }
    GLhandleARB shaderID = glCreateShaderObjectARB(type);
    QByteArray source = file.readAll();
    const char* sdata = source.constData();
    glShaderSource(shaderID, 1, &sdata, 0);
    glCompileShaderARB(shaderID);
    return shaderID;
}

void Environment::init() {
    _skyFromAtmosphereProgramID = glCreateProgramObjectARB();
    glAttachObjectARB(_skyFromAtmosphereProgramID, compileShader(
        GL_VERTEX_SHADER_ARB, "resources/shaders/SkyFromAtmosphere.vert"));
    glAttachObjectARB(_skyFromAtmosphereProgramID, compileShader(
        GL_FRAGMENT_SHADER_ARB, "resources/shaders/SkyFromAtmosphere.frag"));
    glLinkProgramARB(_skyFromAtmosphereProgramID);
    
    _cameraPosLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "v3CameraPos");
    _lightPosLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "v3LightPos");
    _invWavelengthLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "v3InvWavelength");
    _innerRadiusLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fInnerRadius");
    _krESunLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fKrESun");
    _kmESunLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fKmESun");
    _kr4PiLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fKr4PI");
    _km4PiLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fKm4PI");
    _scaleLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fScale");
    _scaleDepthLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fScaleDepth");
    _scaleOverScaleDepthLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "fScaleOverScaleDepth");
    _gLocation = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "g");
    _g2Location = glGetUniformLocationARB(_skyFromAtmosphereProgramID, "g2");
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
    glUseProgramObjectARB(_skyFromAtmosphereProgramID);
    glUniform3fARB(_cameraPosLocation, relativeCameraPos.x, relativeCameraPos.y, relativeCameraPos.z);
    glm::vec3 lightDirection = glm::normalize(getSunLocation());
    glUniform3fARB(_lightPosLocation, lightDirection.x, lightDirection.y, lightDirection.z);
    glUniform3fARB(_invWavelengthLocation,
        1 / powf(getScatteringWavelengths().r, 4.0f),
        1 / powf(getScatteringWavelengths().g, 4.0f),
        1 / powf(getScatteringWavelengths().b, 4.0f));
    glUniform1fARB(_innerRadiusLocation, getAtmosphereInnerRadius());
    glUniform1fARB(_krESunLocation, getRayleighScattering() * getSunBrightness());
    glUniform1fARB(_kmESunLocation, getMieScattering() * getSunBrightness());
    glUniform1fARB(_kr4PiLocation, getRayleighScattering() * 4.0f * PIf);
    glUniform1fARB(_km4PiLocation, getMieScattering() * 4.0f * PIf);
    glUniform1fARB(_scaleLocation, 1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius()));
    glUniform1fARB(_scaleDepthLocation, 0.25f);
    glUniform1fARB(_scaleOverScaleDepthLocation,
        (1.0f / (getAtmosphereOuterRadius() - getAtmosphereInnerRadius())) / 0.25f);
    glUniform1fARB(_gLocation, -0.990f);
    glUniform1fARB(_g2Location, -0.990f * -0.990f);
    
    glFrontFace(GL_CW);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glutSolidSphere(getAtmosphereOuterRadius(), 100, 50);
    glDepthMask(GL_TRUE);
    glFrontFace(GL_CCW);
    
    glUseProgramObjectARB(0);
    
    glPopMatrix();
       
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();   
}
