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

#include "Context.h"

using namespace gpu;

Shader::ProgramMap Shader::_programMap;


Shader::Shader(Type type, const Source& source, bool dynamic) :
    _type(type)
{
    auto& thisSource = const_cast<Source&>(_source);
    thisSource = source;
    if (!dynamic) {
        thisSource.id = source.id;
    }
}

Shader::Shader(Type type, const Pointer& vertex, const Pointer& geometry, const Pointer& pixel) :
    _type(type) 
{

    auto& shaders = const_cast<Shaders&>(_shaders);
    if (geometry) {
        shaders.resize(3);
        shaders[VERTEX] = vertex;
        shaders[GEOMETRY] = geometry;
        shaders[PIXEL] = pixel;
    } else {
        shaders.resize(2);
        shaders[VERTEX] = vertex;
        shaders[PIXEL] = pixel;
    }
}

Shader::Reflection Shader::getReflection(shader::Dialect dialect, shader::Variant variant) const {
    if (_type == Shader::Type::PROGRAM) {
        Reflection reflection;
        for (const auto& subShader : _shaders) {
            reflection.merge(subShader->getReflection(dialect, variant));
        }
        if (_shaders[VERTEX]) {
            reflection.inputs = _shaders[VERTEX]->getReflection(dialect, variant).inputs;
        }
        if (_shaders[PIXEL]) {
            reflection.outputs = _shaders[PIXEL]->getReflection(dialect, variant).outputs;
        }

        return reflection;
    }

    return _source.getReflection(dialect, variant);
}


Shader::Reflection Shader::getReflection() const {
    // For sake of convenience i would like to be able to use a  "default" dialect that represents the reflection
    // of the source of the shader
    // What i really want, is a reflection that is designed for the gpu lib interface but we don't have that yet (glsl45 is the closest to that)
    // Until we have an implementation for this, we will return such default reflection from the one available and platform specific
    return getReflection(shader::DEFAULT_DIALECT, shader::Variant::Mono);
}

Shader::~Shader()
{
}

static std::unordered_map<uint32_t, std::weak_ptr<Shader>> _shaderCache;

Shader::ID Shader::getID() const {
    if (isProgram()) {
        return (_shaders[VERTEX]->getID() << 16) | (_shaders[PIXEL]->getID());
    } 

    return _source.id;
}

Shader::Pointer Shader::createOrReuseDomainShader(Type type, uint32_t sourceId) {
    // Don't attempt to cache non-static shaders
    auto found = _shaderCache.find(sourceId);
    if (found != _shaderCache.end()) {
        auto sharedShader = (*found).second.lock();
        if (sharedShader) {
            return sharedShader;
        }
    }
    auto shader = Pointer(new Shader(type, getShaderSource(sourceId), false));
    _shaderCache.insert({ sourceId, shader });
    return shader;
}


ShaderPointer Shader::createOrReuseProgramShader(Type type, const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader) {
    PROFILE_RANGE(app, "createOrReuseProgramShader");
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

void Shader::setCompilationLogs(const CompilationLogs& logs) const {
    _compilationLogs.clear();
    for (const auto& log : logs) {
        _compilationLogs.emplace_back(CompilationLog(log));
    }
}

void Shader::incrementCompilationAttempt() const {
    _numCompilationAttempts++;
}

Shader::Pointer Shader::createVertex(const Source& source) {
    return Pointer(new Shader(VERTEX, source, true));
}

Shader::Pointer Shader::createPixel(const Source& source) {
    return Pointer(new Shader(FRAGMENT, source, true));
}

Shader::Pointer Shader::createVertex(uint32_t id) {
    return createOrReuseDomainShader(VERTEX, id);
}

Shader::Pointer Shader::createPixel(uint32_t id) {
    return createOrReuseDomainShader(FRAGMENT, id);
}


Shader::Pointer Shader::createProgram(uint32_t programId) {
    auto vertexShader = createVertex(shader::getVertexId(programId));
    auto fragmentShader = createPixel(shader::getFragmentId(programId));
    return createOrReuseProgramShader(PROGRAM, vertexShader, nullptr, fragmentShader);
}

Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& pixelShader) {
    return Pointer(new Shader(PROGRAM, vertexShader, nullptr, pixelShader));
}

// Dynamic program, bypass caching
Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader) {
    return Pointer(new Shader(PROGRAM, vertexShader, geometryShader, pixelShader));
}

const Shader::Source& Shader::getShaderSource(uint32_t id) {
    return shader::Source::get(id);
}
