//
//  PointShader.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 10/30/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

// include this before QOpenGLFramebufferObject, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QOpenGLFramebufferObject>

#include "Application.h"
#include "PointShader.h"
#include "ProgramObject.h"
#include "RenderUtil.h"

PointShader::PointShader()
    : _initialized(false)
{
    _program = NULL;
}

PointShader::~PointShader() {
    if (_initialized) {
        delete _program;
    }
}

ProgramObject* PointShader::createPointShaderProgram(const QString& name) {
    ProgramObject* program = new ProgramObject();
    program->addShaderFromSourceFile(QGLShader::Vertex, "resources/shaders/" + name + ".vert" );
    program->link();
    return program;
}

void PointShader::init() {
    if (_initialized) {
        qDebug("[ERROR] PointShader is already initialized.\n");
        return;
    }
    switchToResourcesParentIfRequired();
    _program = createPointShaderProgram("point_size");
    _initialized = true;
}

void PointShader::begin() {
    _program->bind();
}

void PointShader::end() {
    _program->release();
}

int PointShader::attributeLocation(const char* name) const {
    if (_program) {
        return _program->attributeLocation(name);
    } else {
        return -1;
    }
}

int PointShader::uniformLocation(const char* name) const {
    if (_program) {
        return _program->uniformLocation(name);
    } else {
        return -1;
    }
}

void PointShader::setUniformValue(int uniformLocation, float value) {
    _program->setUniformValue(uniformLocation, value);
}

void PointShader::setUniformValue(int uniformLocation, const glm::vec3& value) {
    _program->setUniformValue(uniformLocation, value.x, value.y, value.z);
}
