//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLShader_h
#define hifi_gpu_gl_GLShader_h

#include "GLShared.h"
#include <gl/GLShaders.h>

namespace gpu { namespace gl {

struct ShaderObject {
    enum class BindingType
    {
        INPUT,
        OUTPUT,
        TEXTURE,
        SAMPLER,
        UNIFORM_BUFFER,
        RESOURCE_BUFFER,
        UNIFORM,
    };

    using LocationMap = std::unordered_map<std::string, int32_t>;
    using ReflectionMap = std::map<BindingType, LocationMap>;
    using UniformMap = std::unordered_map<GLuint, GLuint>;

    GLuint glshader{ 0 };
    GLuint glprogram{ 0 };

    UniformMap uniformRemap;
};

class GLShader : public GPUObject {
public:
    static GLShader* sync(GLBackend& backend, const Shader& shader, const Shader::CompilationHandler& handler = nullptr);
    using ShaderObject = gpu::gl::ShaderObject;
    using ShaderObjects = std::array<ShaderObject, shader::NUM_VARIANTS>;

    GLShader(const std::weak_ptr<GLBackend>& backend);
    ~GLShader();

    ShaderObjects _shaderObjects;

    GLuint getProgram(shader::Variant version = shader::Variant::Mono) const {
        return _shaderObjects[static_cast<uint32_t>(version)].glprogram;
    }

    const std::weak_ptr<GLBackend> _backend;
};

}}  // namespace gpu::gl

#endif
