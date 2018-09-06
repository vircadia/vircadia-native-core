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

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gl {

struct ShaderBinding {
    int index;
    std::string name;
    GLint size{ -1 };
    GLint binding{ -1 };
};

struct Uniform : public ShaderBinding {
    Uniform(){};
    Uniform(GLint program, int index) { load(program, index); };
    using Vector = std::vector<Uniform>;
    GLenum type{ GL_FLOAT };

    void load(GLuint glprogram, int index);
    // Incredibly slow on mac, DO NOT USE
    static Vector load(GLuint glprogram, const std::function<bool(const Uniform&)>& filter);
    static Vector loadTextures(GLuint glprogram);
    static Vector load(GLuint glprogram);
    static Vector load(GLuint glprogram, const std::vector<GLuint>& indices);
    static Vector load(GLuint glprogram, const std::vector<const char*>& names);
    static Vector load(GLuint glprogram, const std::vector<std::string>& names);
    static Uniform loadByName(GLuint glprogram, const std::string& names);

    template <typename C>
    static Vector loadByName(GLuint glprogram, const C& names) {
        if (names.empty()) {
            return {};
        }
        std::vector<const char*> cnames;
        cnames.reserve(names.size());
        for (const auto& name : names) {
            cnames.push_back(name.c_str());
        }
        return load(glprogram, cnames);
    }
};

using Uniforms = Uniform::Vector;

struct UniformBlock : public ShaderBinding {
    UniformBlock(){};
    UniformBlock(GLint program, int index) { load(program, index); };

    using Vector = std::vector<UniformBlock>;
    void load(GLuint glprogram, int index);
    static Vector load(GLuint glprogram);
};

using UniformBlocks = UniformBlock::Vector;

struct Input : public ShaderBinding {
    Input(){};
    Input(GLint program, int index) { load(program, index); };
    using Vector = std::vector<Uniform>;
    GLenum type{ GL_FLOAT };

    void load(GLuint glprogram, int index);
    static Vector load(GLuint glprogram);
};

using Inputs = Input::Vector;

struct CachedShader {
    GLenum format{ 0 };
    std::string source;
    std::vector<char> binary;
    inline operator bool() const { return format != 0 && !binary.empty(); }
};

using ShaderCache = std::unordered_map<std::string, CachedShader>;

std::string getShaderHash(const std::string& shaderSource);
void loadShaderCache(ShaderCache& cache);
void saveShaderCache(const ShaderCache& cache);

#ifdef SEPARATE_PROGRAM
bool compileShader(GLenum shaderDomain,
                   const std::string& shaderSource,
                   GLuint& shaderObject,
                   GLuint& programObject,
                   std::string& message);
bool compileShader(GLenum shaderDomain,
                   const std::vector<std::string>& shaderSources,
                   GLuint& shaderObject,
                   GLuint& programObject,
                   std::string& message);
#else
bool compileShader(GLenum shaderDomain, const std::string& shaderSource, GLuint& shaderObject, std::string& message);
bool compileShader(GLenum shaderDomain,
                   const std::vector<std::string>& shaderSources,
                   GLuint& shaderObject,
                   std::string& message);
#endif

GLuint buildProgram(const std::vector<GLuint>& glshaders);
GLuint buildProgram(const CachedShader& binary);
bool linkProgram(GLuint glprogram, std::string& message);
void getShaderInfoLog(GLuint glshader, std::string& message);
void getProgramInfoLog(GLuint glprogram, std::string& message);
void getProgramBinary(GLuint glprogram, CachedShader& cachedShader);
}  // namespace gl

#endif
