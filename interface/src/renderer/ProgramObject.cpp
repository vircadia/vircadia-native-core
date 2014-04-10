//
//  ProgramObject.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ProgramObject.h"

ProgramObject::ProgramObject(QObject* parent) : QGLShaderProgram(parent) {
}

void ProgramObject::setUniform(int location, const glm::vec3& value) {
    setUniformValue(location, value.x, value.y, value.z);
}

void ProgramObject::setUniform(const char* name, const glm::vec3& value) {
    setUniformValue(name, value.x, value.y, value.z);
}

