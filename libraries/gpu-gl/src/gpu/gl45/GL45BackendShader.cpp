//
//  Created by Sam Gateau on 2017/04/13
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"
#include <gpu/gl/GLShader.h>

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

shader::Dialect GL45Backend::getShaderDialect() const {
#if defined(Q_OS_MAC)
    // We build, but don't actually use GL 4.5 on OSX
    throw std::runtime_error("GL 4.5 unavailable on OSX");
#else
    return shader::Dialect::glsl450;
#endif
}
