//
//  Created by Sam Gateau on 2017/04/13
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"
#include <gpu/gl/GLShader.h>
//#include <gl/GLShaders.h>

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

// GLSL version
std::string GL45Backend::getBackendShaderHeader() const {
    static const std::string header(
        R"SHADER(#version 450 core
        #define GPU_GL450
        #define BITFIELD int
        )SHADER"
#ifdef GPU_SSBO_TRANSFORM_OBJECT
        R"SHADER(#define GPU_SSBO_TRANSFORM_OBJECT)SHADER"
#endif
    );
    return header;
}
