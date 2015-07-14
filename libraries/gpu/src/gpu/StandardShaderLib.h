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

    // Shader draw the unit quad objectPos = ([(-1,-1),(1,1)]) and transform it by the full model transform stack (Model, View, Proj). 
    // A texcoord attribute is also generated texcoord = [(0,0),(1,1)]
    static ShaderPointer getDrawTransformUnitQuadVS();

    // Shader draws the unit quad in the full viewport clipPos = ([(-1,-1),(1,1)]) and transform the texcoord = [(0,0),(1,1)] by the model transform.
    static ShaderPointer getDrawViewportQuadTransformTexcoordVS();

    static ShaderPointer getDrawTexturePS();

protected:

    static ShaderPointer _drawTransformUnitQuadVS;
    static ShaderPointer _drawViewportQuadTransformTexcoordVS;
    static ShaderPointer _drawTexturePS;
};


};

#endif
