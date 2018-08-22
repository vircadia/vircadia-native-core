//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLShader.h"
#include <gl/GLShaders.h>

#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;

GLShader::GLShader(const std::weak_ptr<GLBackend>& backend) : _backend(backend) {
}

GLShader::~GLShader() {
    for (auto& so : _shaderObjects) {
        auto backend = _backend.lock();
        if (backend) {
            if (so.glshader != 0) {
                backend->releaseShader(so.glshader);
            }
            if (so.glprogram != 0) {
                backend->releaseProgram(so.glprogram);
            }
        }
    }
}

GLShader* GLShader::sync(GLBackend& backend, const Shader& shader, const Shader::CompilationHandler& handler) {
    GLShader* object = Backend::getGPUObject<GLShader>(shader);

    // If GPU object already created then good
    if (object) {
        return object;
    }
    // need to have a gpu object?
    if (shader.isProgram()) {
        GLShader* tempObject = backend.compileBackendProgram(shader, handler);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    } else if (shader.isDomain()) {
        GLShader* tempObject = backend.compileBackendShader(shader, handler);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    }

    glFinish();
    return object;
}



