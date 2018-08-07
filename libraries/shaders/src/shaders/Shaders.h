//
//  Created by Bradley Austin Davis on 2018/07/09
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <cstdint>
#include <unordered_map>
#include <string>
#include <shaders/ShaderEnums.h>

namespace shader {

static const uint32_t INVALID_SHADER = (uint32_t)-1;
static const uint32_t INVALID_PROGRAM = (uint32_t)-1;

extern uint32_t all_programs[];

std::string loadShaderSource(uint32_t shaderId);
std::string loadShaderReflection(uint32_t shaderId);

inline uint32_t getVertexId(uint32_t programId) {
    return (programId >> 16) & UINT16_MAX;
}
    
inline uint32_t getFragmentId(uint32_t programId) {
    return programId & UINT16_MAX;
}

}

