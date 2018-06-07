//
//  Created by Bradley Austin Davis 2016/09/27
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_GLShaders_h
#define hifi_GLShaders_h

#include "Config.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace gl {

    struct Uniform {
        std::string name;
        GLint size{ -1 };
        GLenum type{ GL_FLOAT };
        GLint location{ -1 };
        void load(GLuint glprogram, int index);
    };

    using Uniforms = std::vector<Uniform>;

    Uniforms loadUniforms(GLuint glprogram);

    struct CachedShader {
        GLenum format{ 0 };
        std::string source;
        std::vector<char> binary;
        inline operator bool() const {
            return format != 0 && !binary.empty();
        }
    };

    using ShaderCache = std::unordered_map<std::string, CachedShader>;

    std::string getShaderHash(const std::string& shaderSource);
    void loadShaderCache(ShaderCache& cache);
    void saveShaderCache(const ShaderCache& cache);


#ifdef SEPARATE_PROGRAM
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, GLuint &shaderObject, GLuint &programObject, std::string& message);
    bool compileShader(GLenum shaderDomain, const std::vector<std::string>& shaderSources, GLuint &shaderObject, GLuint &programObject, std::string& message);
#else
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, GLuint &shaderObject, std::string& message);
    bool compileShader(GLenum shaderDomain, const std::vector<std::string>& shaderSources, GLuint &shaderObject, std::string& message);
#endif

    GLuint compileProgram(const std::vector<GLuint>& glshaders, std::string& message, CachedShader& binary);

}

#endif
