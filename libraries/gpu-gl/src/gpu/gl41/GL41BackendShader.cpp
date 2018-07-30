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

void GL41Backend::postLinkProgram(ShaderObject& programObject, const Shader& program) const {
    Parent::postLinkProgram(programObject, program);
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


