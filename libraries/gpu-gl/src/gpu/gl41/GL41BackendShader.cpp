//
//  Created by Sam Gateau on 2017/04/13
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"
#include <gpu/gl/GLShader.h>

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl41;

// GLSL version
std::string GL41Backend::getBackendShaderHeader() const {
    static const std::string header(
        R"SHADER(#version 410 core
        #define GPU_GL410
        #define BITFIELD int
        )SHADER");
    return header;
}

template <typename C, typename F>
std::vector<const char*> toCNames(const C& container, F lambda) {
    std::vector<const char*> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(), std::back_inserter(result), lambda);
    return result;
}

#if defined(Q_OS_MAC)
ShaderObject::LocationMap buildRemap(GLuint glprogram, const gpu::Shader::SlotSet& slotSet) {
    static const GLint INVALID_INDEX = -1;
    ShaderObject::LocationMap result;
    const auto expectedNames = slotSet.getNames();
    const auto count = expectedNames.size();
    std::vector<GLint> indices;
    indices.resize(count);
    glGetUniformIndices(glprogram, (GLsizei)count,
                        toCNames(expectedNames, [](const std::string& name) { return name.c_str(); }).data(),
                        (GLuint*)indices.data());

    const auto expectedLocationsByName = slotSet.getLocationsByName();
    for (size_t i = 0; i < count; ++i) {
        const auto& index = indices[i];
        const auto& name = expectedNames[i];
        const auto& expectedLocation = expectedLocationsByName.at(name);
        if (INVALID_INDEX == index) {
            result[expectedLocation] = gpu::Shader::INVALID_LOCATION;
            continue;
        }

        ::gl::Uniform uniformInfo(glprogram, index);
        if (expectedLocation != uniformInfo.binding) {
            result[expectedLocation] = uniformInfo.binding;
        }
    }
    return result;
}
#endif

void GL41Backend::postLinkProgram(ShaderObject& programObject, const Shader& program) const {
    const auto& glprogram = programObject.glprogram;
    // For the UBOs, use glUniformBlockBinding to fixup the locations based on the reflection
    {
        const auto expectedUbos = program.getUniformBuffers().getLocationsByName();
        auto ubos = ::gl::UniformBlock::load(glprogram);
        for (const auto& ubo : ubos) {
            const auto& name = ubo.name;
            if (0 == expectedUbos.count(name)) {
                continue;
            }
            const auto& targetLocation = expectedUbos.at(name);
            if (ubo.binding != targetLocation) {
                glUniformBlockBinding(glprogram, ubo.index, targetLocation);
            }
        }
    }

    // For the Textures, us glUniform1i to fixup the active texture slots based on the reflection
    {
        const auto expectedTextures = program.getTextures().getLocationsByName();
        const auto textureUniforms = ::gl::Uniform::loadByName(glprogram, program.getTextures().getNames());
        for (const auto& texture : textureUniforms) {
            const auto& targetBinding = expectedTextures.at(texture.name);
            glProgramUniform1i(glprogram, texture.binding, targetBinding);
        }
    }

    // For the resource buffers
    {
        const auto expectedTextures = program.getTextures().getLocationsByName();
        const auto textureUniforms = ::gl::Uniform::loadByName(glprogram, program.getTextures().getNames());
        for (const auto& texture : textureUniforms) {
            const auto& targetBinding = expectedTextures.at(texture.name);
            glProgramUniform1i(glprogram, texture.binding, targetBinding);
        }
    }

#if defined(Q_OS_MAC)
    // For the uniforms, we need to create a remapping layer
    programObject.uniformRemap = buildRemap(glprogram, program.getUniforms());
#endif
}


GLint GL41Backend::getRealUniformLocation(GLint location) const {
    GLint result = location;
#if defined(Q_OS_MAC)
    auto& shader = _pipeline._programShader->_shaderObjects[(GLShader::Version)isStereo()];
    auto itr = shader.uniformRemap.find(location);
    if (itr != shader.uniformRemap.end()) {
        result = itr->second;
    }
#endif
    return result;
}
