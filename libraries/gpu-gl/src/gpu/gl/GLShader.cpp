//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLShader.h"
#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;

GLShader::GLShader() {
}

GLShader::~GLShader() {
    for (auto& so : _shaderObjects) {
        if (so.glshader != 0) {
            glDeleteShader(so.glshader);
        }
        if (so.glprogram != 0) {
            glDeleteProgram(so.glprogram);
        }
    }
}

GLShader* compileBackendShader(const Shader& shader) {
    // Any GLSLprogram ? normally yes...
    const std::string& shaderSource = shader.getSource().getCode();

    // GLSL version
    const std::string glslVersion = {
        "#version 410 core"
    };

    // Shader domain
    const int NUM_SHADER_DOMAINS = 2;
    const GLenum SHADER_DOMAINS[NUM_SHADER_DOMAINS] = {
        GL_VERTEX_SHADER,
        GL_FRAGMENT_SHADER
    };
    GLenum shaderDomain = SHADER_DOMAINS[shader.getType()];

    // Domain specific defines
    const std::string domainDefines[NUM_SHADER_DOMAINS] = {
        "#define GPU_VERTEX_SHADER",
        "#define GPU_PIXEL_SHADER"
    };

    // Versions specific of the shader
    const std::string versionDefines[GLShader::NumVersions] = {
        ""
    };

    GLShader::ShaderObjects shaderObjects;

    for (int version = 0; version < GLShader::NumVersions; version++) {
        auto& shaderObject = shaderObjects[version];

        std::string shaderDefines = glslVersion + "\n" + domainDefines[shader.getType()] + "\n" + versionDefines[version];

        bool result = compileShader(shaderDomain, shaderSource, shaderDefines, shaderObject.glshader, shaderObject.glprogram);
        if (!result) {
            return nullptr;
        }
    }

    // So far so good, the shader is created successfully
    GLShader* object = new GLShader();
    object->_shaderObjects = shaderObjects;

    return object;
}

GLShader* compileBackendProgram(const Shader& program) {
    if (!program.isProgram()) {
        return nullptr;
    }

    GLShader::ShaderObjects programObjects;

    for (int version = 0; version < GLShader::NumVersions; version++) {
        auto& programObject = programObjects[version];

        // Let's go through every shaders and make sure they are ready to go
        std::vector< GLuint > shaderGLObjects;
        for (auto subShader : program.getShaders()) {
            auto object = GLShader::sync(*subShader);
            if (object) {
                shaderGLObjects.push_back(object->_shaderObjects[version].glshader);
            } else {
                qCDebug(gpugllogging) << "GLShader::compileBackendProgram - One of the shaders of the program is not compiled?";
                return nullptr;
            }
        }

        GLuint glprogram = compileProgram(shaderGLObjects);
        if (glprogram == 0) {
            return nullptr;
        }

        programObject.glprogram = glprogram;

        makeProgramBindings(programObject);
    }

    // So far so good, the program versions have all been created successfully
    GLShader* object = new GLShader();
    object->_shaderObjects = programObjects;

    return object;
}

GLShader* GLShader::sync(const Shader& shader) {
    GLShader* object = Backend::getGPUObject<GLShader>(shader);

    // If GPU object already created then good
    if (object) {
        return object;
    }
    // need to have a gpu object?
    if (shader.isProgram()) {
        GLShader* tempObject = compileBackendProgram(shader);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    } else if (shader.isDomain()) {
        GLShader* tempObject = compileBackendShader(shader);
        if (tempObject) {
            object = tempObject;
            Backend::setGPUObject(shader, object);
        }
    }

    return object;
}

bool GLShader::makeProgram(Shader& shader, const Shader::BindingSet& slotBindings) {

    // First make sure the Shader has been compiled
    GLShader* object = sync(shader);
    if (!object) {
        return false;
    }

    // Apply bindings to all program versions and generate list of slots from default version
    for (int version = 0; version < GLShader::NumVersions; version++) {
        auto& shaderObject = object->_shaderObjects[version];
        if (shaderObject.glprogram) {
            Shader::SlotSet buffers;
            makeUniformBlockSlots(shaderObject.glprogram, slotBindings, buffers);

            Shader::SlotSet uniforms;
            Shader::SlotSet textures;
            Shader::SlotSet samplers;
            makeUniformSlots(shaderObject.glprogram, slotBindings, uniforms, textures, samplers);

            Shader::SlotSet inputs;
            makeInputSlots(shaderObject.glprogram, slotBindings, inputs);

            Shader::SlotSet outputs;
            makeOutputSlots(shaderObject.glprogram, slotBindings, outputs);

            // Define the public slots only from the default version
            if (version == 0) {
                shader.defineSlots(uniforms, buffers, textures, samplers, inputs, outputs);
            } else {
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

