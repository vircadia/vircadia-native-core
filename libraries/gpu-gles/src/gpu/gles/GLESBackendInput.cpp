//
//  GLESBackendInput.cpp
//  libraries/gpu-gl-android/src/gpu/gles
//
//  Created by Cristian Duarte & Gabriel Calero on 10/7/2016.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLESBackend.h"

using namespace gpu;
using namespace gpu::gles;

void GLESBackend::updateInput() {
    Parent::updateInput();
}


void GLESBackend::resetInputStage() {
    Parent::resetInputStage();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    for (uint32_t i = 0; i < _input._attributeActivation.size(); i++) {
        glDisableVertexAttribArray(i);
        glVertexAttribPointer(i, 4, GL_FLOAT, GL_FALSE, 0, 0);
    }
}