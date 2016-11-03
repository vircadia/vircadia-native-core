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

Shader::Shader(Type type, const Pointer& vertex, const Pointer& pixel):
    _type(type)
{
    _shaders.resize(2);
    _shaders[VERTEX] = vertex;
    _shaders[PIXEL] = pixel;
}

Shader::Shader(Type type, const Pointer& vertex, const Pointer& geometry, const Pointer& pixel) :
_type(type) {
    _shaders.resize(3);
    _shaders[VERTEX] = vertex;
    _shaders[GEOMETRY] = geometry;
    _shaders[PIXEL] = pixel;
}

Shader::~Shader()
{
}

Shader::Pointer Shader::createVertex(const Source& source) {
    return Pointer(new Shader(VERTEX, source));
}

Shader::Pointer Shader::createPixel(const Source& source) {
    return Pointer(new Shader(PIXEL, source));
}

Shader::Pointer Shader::createGeometry(const Source& source) {
    return Pointer(new Shader(GEOMETRY, source));
}

Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& pixelShader) {
    if (vertexShader && vertexShader->getType() == VERTEX &&
        pixelShader && pixelShader->getType() == PIXEL) {
        return Pointer(new Shader(PROGRAM, vertexShader, pixelShader));
    }
    return Pointer();
}

Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader) {
    if (vertexShader && vertexShader->getType() == VERTEX &&
        geometryShader && geometryShader->getType() == GEOMETRY &&
        pixelShader && pixelShader->getType() == PIXEL) {
        return Pointer(new Shader(PROGRAM, vertexShader, geometryShader, pixelShader));
    }
    return Pointer();
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
