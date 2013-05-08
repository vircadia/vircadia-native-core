//
//  ShaderObject.cpp
//  interface
//
//  Created by Andrzej Kapolka on 5/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <QFile>

#include "ShaderObject.h"

ShaderObject::ShaderObject(int type)
    : _handle(glCreateShaderObjectARB(type)) {
}

ShaderObject::~ShaderObject() {
    glDeleteObjectARB(_handle);
}

bool ShaderObject::compileSourceCode(const char* data) {
    glShaderSourceARB(_handle, 1, &data, 0);
    glCompileShaderARB(_handle);
    int status;
    glGetObjectParameterivARB(_handle, GL_OBJECT_COMPILE_STATUS_ARB, &status);
    return status;
}

bool ShaderObject::compileSourceFile(const char* filename) {
    QFile file(filename);
    return file.open(QIODevice::ReadOnly) && compileSourceCode(file.readAll().constData());
}

QByteArray ShaderObject::getLog() const {
    int length;
    glGetObjectParameterivARB(_handle, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    QByteArray log(length, 0);
    glGetInfoLogARB(_handle, length, 0, log.data());
    return log;
}
