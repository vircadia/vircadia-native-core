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
#include <map>

#include "Shader.h"

namespace gpu {

class StandardShaderLib {
public:

    // Shader draws the unit quad in the full viewport clipPos = ([(-1,-1),(1,1)]) and the unit texcoord = [(0,0),(1,1)].
    static ShaderPointer getDrawUnitQuadTexcoordVS();

    // Shader draw the unit quad objectPos = ([(-1,-1),(1,1)]) and transform it by the full model transform stack (Model, View, Proj). 
    // A texcoord attribute is also generated texcoord = [(0,0),(1,1)]
    static ShaderPointer getDrawTransformUnitQuadVS();

    // Shader draw the unit quad objectPos = ([(-1,-1),(1,1)]) and transform it by the full model transform stack (Model, View, Proj). 
    // A texcoord attribute is also generated covering a rect defined from the uniform vec4 texcoordRect: texcoord = [texcoordRect.xy,texcoordRect.xy + texcoordRect.zw]
    static ShaderPointer getDrawTexcoordRectTransformUnitQuadVS();

    // Shader draws the unit quad in the full viewport clipPos = ([(-1,-1),(1,1)]) and transform the texcoord = [(0,0),(1,1)] by the model transform.
    static ShaderPointer getDrawViewportQuadTransformTexcoordVS();

    // Shader draw the fed vertex position and transform it by the full model transform stack (Model, View, Proj).
    // simply output the world pos and the clip pos to the next stage
    static ShaderPointer getDrawVertexPositionVS();
    static ShaderPointer getDrawTransformVertexPositionVS();

    // PShader does nothing, no really nothing, but still needed for defining a program triggering rasterization
    static ShaderPointer getDrawNadaPS();

    static ShaderPointer getDrawWhitePS();
    static ShaderPointer getDrawTexturePS();
    static ShaderPointer getDrawTextureOpaquePS();
    static ShaderPointer getDrawColoredTexturePS();

    // The shader program combining the shaders available above, so they are unique
    typedef ShaderPointer (*GetShader) ();
    static ShaderPointer getProgram(GetShader vs, GetShader ps);

protected:

    static ShaderPointer _drawUnitQuadTexcoordVS;
    static ShaderPointer _drawTransformUnitQuadVS;
    static ShaderPointer _drawTexcoordRectTransformUnitQuadVS;
    static ShaderPointer _drawViewportQuadTransformTexcoordVS;

    static ShaderPointer _drawVertexPositionVS;
    static ShaderPointer _drawTransformVertexPositionVS;

    static ShaderPointer _drawNadaPS;
    static ShaderPointer _drawWhitePS;
    static ShaderPointer _drawTexturePS;
    static ShaderPointer _drawTextureOpaquePS;
    static ShaderPointer _drawColoredTexturePS;

    typedef std::map<std::pair<GetShader, GetShader>, ShaderPointer> ProgramMap;
    static ProgramMap _programs;
};


};

#endif
