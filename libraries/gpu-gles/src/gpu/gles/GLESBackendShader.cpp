//
//  Created by Sam Gateau on 2017/04/13
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLESBackend.h"
#include <gpu/gl/GLShader.h>

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gles;

// GLSL version
std::string GLESBackend::getBackendShaderHeader() const {
    static const std::string header(
        R"SHADER(#version 310 es
        #extension GL_EXT_texture_buffer : enable
        precision highp float;
        precision highp samplerBuffer;
        precision highp sampler2DShadow;
        #define BITFIELD highp int
        )SHADER");
    return header;
}
