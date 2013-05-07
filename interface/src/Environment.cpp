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

// checks for an error, printing the info log if one is deteced
static void errorCheck(GLhandleARB obj) {
    if (glGetError() != GL_NO_ERROR) {
        QByteArray log(1024, 0);
        glGetInfoLogARB(obj, log.size(), 0, log.data());
        qDebug() << log;
    }
}

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
    errorCheck(shaderID);
    return shaderID;
}

void Environment::init() {
    _skyFromAtmosphereProgramID = glCreateProgramObjectARB();
    glAttachObjectARB(_skyFromAtmosphereProgramID, compileShader(
        GL_VERTEX_SHADER_ARB, "resources/shaders/SkyFromAtmosphere.vert"));
    glAttachObjectARB(_skyFromAtmosphereProgramID, compileShader(
        GL_FRAGMENT_SHADER_ARB, "resources/shaders/SkyFromAtmosphere.frag"));
    glLinkProgramARB(_skyFromAtmosphereProgramID);
    errorCheck(_skyFromAtmosphereProgramID);
    
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

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(camera.getFieldOfView(), camera.getAspectRatio(), 100, 100000000);
    
    glUseProgramObjectARB(_skyFromAtmosphereProgramID);
    glm::vec3 relativeCameraPos = camera.getPosition() - getAtmosphereCenter();
    glUniform3fARB(_cameraPosLocation, relativeCameraPos.x, relativeCameraPos.y, relativeCameraPos.z);
    glm::vec3 lightDirection = glm::normalize(getSunLocation());
    glUniform3fARB(_lightPosLocation, lightDirection.x, lightDirection.y, lightDirection.z);
    glUniform3fARB(_invWavelengthLocation,
        1 / powf(0.650f, 4.0f), 1 / powf(0.570f, 4.0f), 1 / powf(0.475f, 4.0f));
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
    glutSolidSphere(getAtmosphereOuterRadius(), 100, 50);
    glFrontFace(GL_CCW);
    
    glUseProgramObjectARB(0);
    
    glPopMatrix();
       
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
