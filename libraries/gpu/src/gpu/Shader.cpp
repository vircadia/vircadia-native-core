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

#include "Context.h"

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

void Shader::defineSlots(const SlotSet& uniforms, const SlotSet& buffers, const SlotSet& textures, const SlotSet& samplers, const SlotSet& inputs, const SlotSet& outputs) {
    _uniforms = uniforms;
    _buffers = buffers;
    _textures = textures;
    _samplers = samplers;
    _inputs = inputs;
    _outputs = outputs;
}

bool Shader::makeProgram(Shader& shader, const Shader::BindingSet& bindings) {
    if (shader.isProgram()) {
        return Context::makeProgram(shader, bindings);
    }
    return false;
}
