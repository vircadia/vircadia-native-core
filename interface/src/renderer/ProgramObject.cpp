//
//  ProgramObject.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "ProgramObject.h"

ProgramObject::ProgramObject(QObject* parent) : QGLShaderProgram(parent) {
}

void ProgramObject::setUniform(int location, const glm::vec3& value) {
    setUniformValue(location, value.x, value.y, value.z);
}

void ProgramObject::setUniform(const char* name, const glm::vec3& value) {
    setUniformValue(name, value.x, value.y, value.z);
}

