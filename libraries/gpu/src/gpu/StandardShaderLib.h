//
//  StandardShaderLib.h
//  libraries/gpu/src/gpu
//
//  Collection of standard shaders that can be used all over the place
//
//  Created by Sam Gateau on 6/22/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_StandardShaderLib_h
#define hifi_gpu_StandardShaderLib_h

#include <assert.h>

#include "Shader.h"

namespace gpu {

class StandardShaderLib {
public:

    static ShaderPointer getDrawTransformUnitQuadVS();

    static ShaderPointer getDrawTexturePS();

protected:

    static ShaderPointer _drawTransformUnitQuadVS;
    static ShaderPointer _drawTexturePS;
};


};

#endif
