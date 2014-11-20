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
#include <glm/gtc/type_ptr.hpp>

ProgramObject::ProgramObject(QObject* parent) : QGLShaderProgram(parent) {
}

void ProgramObject::setUniform(int location, const glm::vec2& value) {
    setUniformValue(location, value.x, value.y);
}

void ProgramObject::setUniform(const char* name, const glm::vec2& value) {
    setUniformValue(name, value.x, value.y);
}

void ProgramObject::setUniform(int location, const glm::vec3& value) {
    setUniformValue(location, value.x, value.y, value.z);
}

void ProgramObject::setUniform(const char* name, const glm::vec3& value) {
    setUniformValue(name, value.x, value.y, value.z);
}

void ProgramObject::setUniform(int location, const glm::vec4& value) {
    setUniformValue(location, value.x, value.y, value.z, value.w);
}

void ProgramObject::setUniform(const char* name, const glm::vec4& value) {
    setUniformValue(name, value.x, value.y, value.z, value.w);
}

void ProgramObject::setUniformArray(const char* name, const glm::vec3* values, int count) {
    GLfloat* floatVal = new GLfloat[count*3];
    int index = 0;
    for (int i = 0; i < count; i++) {
        assert(index < count*3);
        const float* valPtr = glm::value_ptr(values[i]);
        floatVal[index++] = valPtr[0];
        floatVal[index++] = valPtr[1];
        floatVal[index++] = valPtr[2];
    }
    
    setUniformValueArray(name, floatVal, count, 3);
    delete[] floatVal;
}
