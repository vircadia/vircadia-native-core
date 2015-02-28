//
//  Shader.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 2/27/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shader.h"
#include <math.h>
#include <QDebug>

using namespace gpu;

Shader::Shader(Type type, const Source& source):
    _source(source),
    _type(type)
{
}

Shader::Shader(Type type, Pointer& vertex, Pointer& pixel):
    _type(type)
{
    _shaders.resize(2);
    _shaders[VERTEX] = vertex;
    _shaders[PIXEL] = pixel;
}


Shader::~Shader()
{
}

/*
Program::Program():
    _storage(),
    _type(GRAPHICS)
{
}

Program::~Program()
{
}
*/

Shader* Shader::createVertex(const Source& source) {
    Shader* shader = new Shader(VERTEX, source);
    return shader;
}

Shader* Shader::createPixel(const Source& source) {
    Shader* shader = new Shader(PIXEL, source);
    return shader;
}

Shader* Shader::createProgram(Pointer& vertexShader, Pointer& pixelShader) {
    if (vertexShader && vertexShader->getType() == VERTEX) {
        if (pixelShader && pixelShader->getType() == PIXEL) {
            Shader* shader = new Shader(PROGRAM, vertexShader, pixelShader);
            return shader;
        }
    }
    return nullptr;
}
