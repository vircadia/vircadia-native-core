//
//  StandardShaderLib.cpp
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
#include "StandardShaderLib.h"

#include "DrawTransformUnitQuad_vert.h"
#include "DrawViewportQuadTransformTexcoord_vert.h"
#include "DrawTexture_frag.h"

using namespace gpu;

ShaderPointer StandardShaderLib::_drawTransformUnitQuadVS;
ShaderPointer StandardShaderLib::_drawViewportQuadTransformTexcoordVS;
ShaderPointer StandardShaderLib::_drawTexturePS;

ShaderPointer StandardShaderLib::getDrawTransformUnitQuadVS() {
    if (!_drawTransformUnitQuadVS) {
        _drawTransformUnitQuadVS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(DrawTransformUnitQuad_vert)));
    }
    return _drawTransformUnitQuadVS;
}

ShaderPointer StandardShaderLib::getDrawViewportQuadTransformTexcoordVS() {
    if (!_drawViewportQuadTransformTexcoordVS) {
        _drawViewportQuadTransformTexcoordVS = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(DrawViewportQuadTransformTexcoord_vert)));
    }
    return _drawViewportQuadTransformTexcoordVS;
}

ShaderPointer StandardShaderLib::getDrawTexturePS() {
    if (!_drawTexturePS) {
        _drawTexturePS = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(DrawTexture_frag)));
    }
    return _drawTexturePS;
}
