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

namespace gl {
#ifdef SEPARATE_PROGRAM
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject, GLuint &programObject);
#else
    bool compileShader(GLenum shaderDomain, const std::string& shaderSource, const std::string& defines, GLuint &shaderObject);
#endif

    GLuint compileProgram(const std::vector<GLuint>& glshaders);

}

#endif
