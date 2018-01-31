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

GLShader* GLShader::sync(GLBackend& backend, const Shader& shader) {
    GLShader* object = Backend::getGPUObject<GLShader>(shader);

    // If GPU object already created then good
    if (object) {
        return object;
    }
    // need to have a gpu object?
    if (shader.isProgram()) {
        GLShader* tempObject = backend.compileBackendProgram(shader);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    } else if (shader.isDomain()) {
        GLShader* tempObject = backend.compileBackendShader(shader);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    }

    glFinish();
    return object;
}

bool GLShader::makeProgram(GLBackend& backend, Shader& shader, const Shader::BindingSet& slotBindings) {

    // First make sure the Shader has been compiled
    GLShader* object = sync(backend, shader);
    if (!object) {
        return false;
    }

    // Apply bindings to all program versions and generate list of slots from default version
    for (int version = 0; version < GLShader::NumVersions; version++) {
        auto& shaderObject = object->_shaderObjects[version];
        if (shaderObject.glprogram) {
            Shader::SlotSet buffers;
            backend.makeUniformBlockSlots(shaderObject.glprogram, slotBindings, buffers);

            Shader::SlotSet uniforms;
            Shader::SlotSet textures;
            Shader::SlotSet samplers;
            backend.makeUniformSlots(shaderObject.glprogram, slotBindings, uniforms, textures, samplers);

            Shader::SlotSet resourceBuffers;
            backend.makeResourceBufferSlots(shaderObject.glprogram, slotBindings, resourceBuffers);

            Shader::SlotSet inputs;
            backend.makeInputSlots(shaderObject.glprogram, slotBindings, inputs);

            Shader::SlotSet outputs;
            backend.makeOutputSlots(shaderObject.glprogram, slotBindings, outputs);

            // Define the public slots only from the default version
            if (version == 0) {
                shader.defineSlots(uniforms, buffers, resourceBuffers, textures, samplers, inputs, outputs);
            } // else
            {
                GLShader::UniformMapping mapping;
                for (auto srcUniform : shader.getUniforms()) {
                    mapping[srcUniform._location] = uniforms.findLocation(srcUniform._name);
                }
                object->_uniformMappings.push_back(mapping);
            }
        }
    }

    return true;
}



