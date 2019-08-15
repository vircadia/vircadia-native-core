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

void GL41Backend::postLinkProgram(ShaderObject& programObject, const Shader& program) const {
    Parent::postLinkProgram(programObject, program);
    const auto& glprogram = programObject.glprogram;
    auto reflection = program.getReflection(getShaderDialect(), getShaderVariant());
    // For the UBOs, use glUniformBlockBinding to fixup the locations based on the reflection
    {
        const auto& expectedUbos = reflection.uniformBuffers;
        auto ubos = ::gl::UniformBlock::load(glprogram);
        for (const auto& ubo : ubos) {
            const auto& name = ubo.name;
            if (0 == expectedUbos.count(name)) {
                continue;
            }
            const auto& targetLocation = expectedUbos.at(name);
            glUniformBlockBinding(glprogram, ubo.index, targetLocation);
        }
    }

    // For the Textures, use glUniform1i to fixup the active texture slots based on the reflection
    {
        const auto& expectedTextures = reflection.textures;
        for (const auto& expectedTexture : expectedTextures) {
            auto location = glGetUniformLocation(glprogram, expectedTexture.first.c_str());
            if (location < 0) {
                continue;
            }
            glProgramUniform1i(glprogram, location, expectedTexture.second);
        }
    }

    // For the resource buffers, do the same as for the textures, since in GL 4.1 that's how they're implemented
    static const std::string TRANSFORM_OBJECT_BUFFER = "transformObjectBuffer";
    {
        const auto& expectedResourceBuffers = reflection.resourceBuffers;
        const auto names = Shader::Reflection::getNames(expectedResourceBuffers);
        const auto resourceBufferUniforms = ::gl::Uniform::loadByName(glprogram, names);
        for (const auto& resourceBuffer : resourceBufferUniforms) {

            // Special case for the transformObjectBuffer, which is filtered out of the reflection data at shader load time
            if (resourceBuffer.name == TRANSFORM_OBJECT_BUFFER) {
                glProgramUniform1i(glprogram, resourceBuffer.binding, ::gpu::slot::texture::ObjectTransforms);
                continue;
            }
            const auto& targetBinding = expectedResourceBuffers.at(resourceBuffer.name);
            glProgramUniform1i(glprogram, resourceBuffer.binding, targetBinding);
        }
    }

}


