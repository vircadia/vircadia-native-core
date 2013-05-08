//
//  ProgramObject.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include "ProgramObject.h"
#include "ShaderObject.h"

ProgramObject::ProgramObject() : _handle(glCreateProgramObjectARB()) {
}

ProgramObject::~ProgramObject() {
    glDeleteObjectARB(_handle);
}

void ProgramObject::attach(ShaderObject* shader) {
    glAttachObjectARB(_handle, shader->getHandle());
}

bool ProgramObject::attachFromSourceCode(int type, const char* source) {
    ShaderObject* shader = new ShaderObject(type);
    if (shader->compileSourceCode(source)) {
        attach(shader);
        return true;
        
    } else {
        delete shader;
        return false;
    }
}

bool ProgramObject::attachFromSourceFile(int type, const char* filename) {
    ShaderObject* shader = new ShaderObject(type);
    if (shader->compileSourceFile(filename)) {
        attach(shader);
        return true;
        
    } else {
        delete shader;
        return false;
    }
}

bool ProgramObject::link() {
    glLinkProgramARB(_handle);
    int status;
    glGetObjectParameterivARB(_handle, GL_OBJECT_LINK_STATUS_ARB, &status);
    return status;
}

QByteArray ProgramObject::getLog() const {
    int length;
    glGetObjectParameterivARB(_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    QByteArray log(length, 0);
    glGetInfoLogARB(_handle, length, 0, log.data());
    return log;
}

void ProgramObject::bind() const {
    glUseProgramObjectARB(_handle);
}

void ProgramObject::release() const {
    glUseProgramObjectARB(0);
}

int ProgramObject::getUniformLocation(const char* name) const {
    return glGetUniformLocationARB(_handle, name);
}

void ProgramObject::setUniform(int location, int value) {
    glUniform1iARB(location, value);
}

void ProgramObject::setUniform(const char* name, int value) {
    setUniform(getUniformLocation(name), value);
}

void ProgramObject::setUniform(int location, float value) {
    glUniform1fARB(location, value);
}

void ProgramObject::setUniform(const char* name, float value) {
    setUniform(getUniformLocation(name), value);
}

void ProgramObject::setUniform(int location, float x, float y) {
    glUniform2fARB(location, x, y);
}

void ProgramObject::setUniform(const char* name, float x, float y) {
    setUniform(getUniformLocation(name), x, y);
}

void ProgramObject::setUniform(int location, const glm::vec3& value) {
    glUniform3fARB(location, value.x, value.y, value.z);
}

void ProgramObject::setUniform(const char* name, const glm::vec3& value) {
    setUniform(getUniformLocation(name), value);
}

void ProgramObject::setUniform(int location, float x, float y, float z) {
    glUniform3fARB(location, x, y, z);
}

void ProgramObject::setUniform(const char* name, float x, float y, float z) {
    setUniform(getUniformLocation(name), x, y, z);
}

void ProgramObject::setUniform(int location, float x, float y, float z, float w) {
    glUniform4fARB(location, x, y, z, w);
}

void ProgramObject::setUniform(const char* name, float x, float y, float z, float w) {
    setUniform(getUniformLocation(name), x, y, z, w);
}
