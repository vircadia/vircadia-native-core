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
#include <set>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

#include <shaders/Shaders.h>

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

void populateSlotSet(Shader::SlotSet& slotSet, const Shader::LocationMap& map) {
    for (const auto& entry : map) {
        const auto& name = entry.first;
        const auto& location = entry.second;
        slotSet.insert({ name, location, Element() });
    }
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
    const auto& reflection = source.getReflection();
    if (0 != reflection.count(BindingType::INPUT)) {
        populateSlotSet(shader->_inputs, reflection.find(BindingType::INPUT)->second);
    }
    if (0 != reflection.count(BindingType::OUTPUT)) {
        populateSlotSet(shader->_outputs, reflection.find(BindingType::OUTPUT)->second);
    }
    if (0 != reflection.count(BindingType::UNIFORM_BUFFER)) {
        populateSlotSet(shader->_uniformBuffers, reflection.find(BindingType::UNIFORM_BUFFER)->second);
    }
    if (0 != reflection.count(BindingType::RESOURCE_BUFFER)) {
        populateSlotSet(shader->_resourceBuffers, reflection.find(BindingType::RESOURCE_BUFFER)->second);
    }
    if (0 != reflection.count(BindingType::TEXTURE)) {
        populateSlotSet(shader->_textures, reflection.find(BindingType::TEXTURE)->second);
    }
    if (0 != reflection.count(BindingType::SAMPLER)) {
        populateSlotSet(shader->_samplers, reflection.find(BindingType::SAMPLER)->second);
    }
    if (0 != reflection.count(BindingType::UNIFORM)) {
        populateSlotSet(shader->_uniforms, reflection.find(BindingType::UNIFORM)->second);
    }
    _domainShaderMaps[type].emplace(source, std::weak_ptr<Shader>(shader));
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

    // Combine the slots from the sub-shaders
    for (const auto& shader : program->_shaders) {
        const auto& reflection = shader->_source.getReflection();
        if (0 != reflection.count(BindingType::UNIFORM_BUFFER)) {
            populateSlotSet(program->_uniformBuffers, reflection.find(BindingType::UNIFORM_BUFFER)->second);
        }
        if (0 != reflection.count(BindingType::RESOURCE_BUFFER)) {
            populateSlotSet(program->_resourceBuffers, reflection.find(BindingType::RESOURCE_BUFFER)->second);
        }
        if (0 != reflection.count(BindingType::TEXTURE)) {
            populateSlotSet(program->_textures, reflection.find(BindingType::TEXTURE)->second);
        }
        if (0 != reflection.count(BindingType::SAMPLER)) {
            populateSlotSet(program->_samplers, reflection.find(BindingType::SAMPLER)->second);
        }
        if (0 != reflection.count(BindingType::UNIFORM)) {
            populateSlotSet(program->_uniforms, reflection.find(BindingType::UNIFORM)->second);
        }

    }

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
    return createOrReuseDomainShader(VERTEX, source);
}

Shader::Pointer Shader::createPixel(const Source& source) {
    return createOrReuseDomainShader(FRAGMENT, source);
}

Shader::Pointer Shader::createVertex(uint32_t id) {
    return createVertex(getShaderSource(id));
}

Shader::Pointer Shader::createPixel(uint32_t id) {
    return createPixel(getShaderSource(id));
}

Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& pixelShader) {
    return createOrReuseProgramShader(PROGRAM, vertexShader, nullptr, pixelShader);
}

Shader::Pointer Shader::createProgram(uint32_t programId) {
    auto vertexShader = createVertex(shader::getVertexId(programId));
    auto fragmentShader = createPixel(shader::getFragmentId(programId));
    return createOrReuseProgramShader(PROGRAM, vertexShader, nullptr, fragmentShader);
}

Shader::Pointer Shader::createProgram(const Pointer& vertexShader, const Pointer& geometryShader, const Pointer& pixelShader) {
    return createOrReuseProgramShader(PROGRAM, vertexShader, geometryShader, pixelShader);
}

static const std::string IGNORED_BINDING = "transformObjectBuffer";

void updateBindingsFromJsonObject(Shader::LocationMap& inOutSet, const QJsonObject& json) {
    for (const auto& key : json.keys()) {
        auto keyStr = key.toStdString();
        if (IGNORED_BINDING == keyStr) {
            continue;
        }
        inOutSet[keyStr] = json[key].toInt();
    }
}

void updateTextureAndResourceBuffersFromJsonObjects(Shader::LocationMap& inOutTextures, Shader::LocationMap& inOutResourceBuffers,
                                                    const QJsonObject& json, const QJsonObject& types) {
    static const std::string RESOURCE_BUFFER_TEXTURE_TYPE = "samplerBuffer";
    for (const auto& key : json.keys()) {
        auto keyStr = key.toStdString();
        if (keyStr == IGNORED_BINDING) {
            continue;
        }
        auto location = json[key].toInt();
        auto type = types[key].toString().toStdString();
        if (type == RESOURCE_BUFFER_TEXTURE_TYPE) {
            inOutResourceBuffers[keyStr] = location;
        } else {
            inOutTextures[key.toStdString()] = location;
        }
    }
}

Shader::ReflectionMap getShaderReflection(const std::string& reflectionJson) {
    if (reflectionJson.empty() && reflectionJson != std::string("null")) {
        return {};
    }

#define REFLECT_KEY_INPUTS "inputs"
#define REFLECT_KEY_OUTPUTS "outputs"
#define REFLECT_KEY_UBOS "uniformBuffers"
#define REFLECT_KEY_SSBOS "storageBuffers"
#define REFLECT_KEY_UNIFORMS "uniforms"
#define REFLECT_KEY_TEXTURES "textures"
#define REFLECT_KEY_TEXTURE_TYPES "textureTypes"

    auto doc = QJsonDocument::fromJson(reflectionJson.c_str());
    if (doc.isNull()) {
        qWarning() << "Invalid shader reflection JSON" << reflectionJson.c_str();
        return {};
    }

    Shader::ReflectionMap result;
    auto json = doc.object();
    if (json.contains(REFLECT_KEY_INPUTS)) {
        updateBindingsFromJsonObject(result[Shader::BindingType::INPUT], json[REFLECT_KEY_INPUTS].toObject());
    }
    if (json.contains(REFLECT_KEY_OUTPUTS)) {
        updateBindingsFromJsonObject(result[Shader::BindingType::OUTPUT], json[REFLECT_KEY_OUTPUTS].toObject());
    }
    // FIXME eliminate the last of the uniforms
    if (json.contains(REFLECT_KEY_UNIFORMS)) {
        updateBindingsFromJsonObject(result[Shader::BindingType::UNIFORM], json[REFLECT_KEY_UNIFORMS].toObject());
    }
    if (json.contains(REFLECT_KEY_UBOS)) {
        updateBindingsFromJsonObject(result[Shader::BindingType::UNIFORM_BUFFER], json[REFLECT_KEY_UBOS].toObject());
    }

    // SSBOs need to come BEFORE the textures.  In GL 4.5 the reflection slots aren't really used, but in 4.1 the slots
    // are used to explicitly setup bindings after shader linkage, so we want the resource buffer slots to contain the
    // texture locations, not the SSBO locations
    if (json.contains(REFLECT_KEY_SSBOS)) {
        updateBindingsFromJsonObject(result[Shader::BindingType::RESOURCE_BUFFER], json[REFLECT_KEY_SSBOS].toObject());
    }

    // samplerBuffer textures map to gpu ResourceBuffer, while all other textures map to regular gpu Texture
    if (json.contains(REFLECT_KEY_TEXTURES)) {
        updateTextureAndResourceBuffersFromJsonObjects(
               result[Shader::BindingType::TEXTURE],
               result[Shader::BindingType::RESOURCE_BUFFER],
               json[REFLECT_KEY_TEXTURES].toObject(),
               json[REFLECT_KEY_TEXTURE_TYPES].toObject());
    }

    
    return result;
}

Shader::Source Shader::getShaderSource(uint32_t id) {
    auto source = shader::loadShaderSource(id);
    auto reflectionJson = shader::loadShaderReflection(id);
    auto reflection = getShaderReflection(reflectionJson);
    return { source, reflection };
}
