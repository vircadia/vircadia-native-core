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

std::atomic<uint32_t> Shader::_nextShaderID( 1 );
Shader::DomainShaderMaps Shader::_domainShaderMaps;
Shader::ProgramMap Shader::_programMap;


Shader::Shader(Type type, const Source& source) :
    _source(source),
    _type(type),
    _ID(_nextShaderID++)
{
}

Shader::Shader(Type type, const Pointer& vertex, const Pointer& geometry, const Pointer& pixel):
    _type(type),
    _ID(_nextShaderID++)
{
    if (geometry) {
        _shaders.resize(3);
        _shaders[VERTEX] = vertex;
        _shaders[GEOMETRY] = geometry;
        _shaders[PIXEL] = pixel;
    } else {
        _shaders.resize(2);
        _shaders[VERTEX] = vertex;
        _shaders[PIXEL] = pixel;
    }
}

Shader::~Shader()
{
}

Shader::Pointer Shader::createOrReuseDomainShader(Type type, const Source& source) {
    auto found = _domainShaderMaps[type].find(source);
    if (found != _domainShaderMaps[type].end()) {
        auto sharedShader = (*found).second.lock();
        if (sharedShader) {
            return sharedShader;
        }
    }
    auto shader = Pointer(new Shader(type, source));
    _domainShaderMaps[type].emplace(source, std::weak_ptr<Shader>(shader));
    return shader;
}

Shader::Pointer Shader::createVertex(const Source& source) {
    return createOrReuseDomainShader(VERTEX, source);
}

Shader::Pointer Shader::createPixel(const Source& source) {
    return createOrReuseDomainShader(PIXEL, source);
}

Shader::Pointer Shader::createGeometry(const Source& source) {
    return createOrReuseDomainShader(GEOMETRY, source);
}

ShaderPointer Shader::createOrReuseProgramShader(Type type, const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader) {
    ProgramMapKey key(0);

    if (vertexShader && vertexShader->getType() == VERTEX) {
        key.x = vertexShader->getID();
    } else {
        // Shader is not valid, exit
        return Pointer();
    }

    if (pixelShader && pixelShader->getType() == PIXEL) {
        key.y = pixelShader->getID();
    } else {
        // Shader is not valid, exit
        return Pointer();
    }

    if (geometryShader) {
        if (geometryShader->getType() == GEOMETRY) {
            key.z = geometryShader->getID();
        } else {
            // Shader is not valid, exit
            return Pointer();
        }
    }

    // program key is defined, now try to reuse
    auto found = _programMap.find(key);
    if (found != _programMap.end()) {
        auto sharedShader = (*found).second.lock();
        if (sharedShader) {
            return sharedShader;
        }
    }

    // Program is a new one, let's create it
    auto program = Pointer(new Shader(type, vertexShader, geometryShader, pixelShader));
    _programMap.emplace(key, std::weak_ptr<Shader>(program));
    return program;
}


Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& pixelShader) {
    return createOrReuseProgramShader(PROGRAM, vertexShader, nullptr, pixelShader);
}

Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader) {
    return createOrReuseProgramShader(PROGRAM, vertexShader, geometryShader, pixelShader);
}

void Shader::defineSlots(const SlotSet& uniforms, const SlotSet& uniformBuffers, const SlotSet& resourceBuffers, const SlotSet& textures, const SlotSet& samplers, const SlotSet& inputs, const SlotSet& outputs) {
    _uniforms = uniforms;
    _uniformBuffers = uniformBuffers;
    _resourceBuffers = resourceBuffers;
    _textures = textures;
    _samplers = samplers;
    _inputs = inputs;
    _outputs = outputs;
}

bool Shader::makeProgram(Shader& shader, const Shader::BindingSet& bindings, const CompilationHandler& handler) {
    if (shader.isProgram()) {
        return Context::makeProgram(shader, bindings, handler);
    }
    return false;
}

void Shader::setCompilationLogs(const CompilationLogs& logs) const {
    _compilationLogs.clear();
    for (const auto& log : logs) {
        _compilationLogs.emplace_back(CompilationLog(log));
    }
}

void Shader::incrementCompilationAttempt() const {
    _numCompilationAttempts++;
}

