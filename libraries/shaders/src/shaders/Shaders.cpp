//
//  Created by Bradley Austin Davis on 2018/06/02
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Shaders.h"

#include <mutex>
#include <regex>
#include <sstream>
#include <unordered_map>

#include <QtCore/QFileInfo>
#include <QtCore/QFile>

#include <nlohmann/json.hpp>

#include <shared/FileUtils.h>
#include <gl/GLHelpers.h>

// Can't use the Q_INIT_RESOURCE macro inside a namespace on Mac,
// so this is done out of line

static void initShadersResources() {
    Q_INIT_RESOURCE(shaders);
}

namespace shader {

#if defined(USE_GLES)

const Dialect DEFAULT_DIALECT = Dialect::glsl310es;

const std::vector<Dialect>& allDialects() {
    static const std::vector<Dialect> ALL_DIALECTS{ Dialect::glsl310es };
    return ALL_DIALECTS;
}
    
#elif defined(Q_OS_MAC) 

const Dialect DEFAULT_DIALECT = Dialect::glsl410;

const std::vector<Dialect>& allDialects() {
    static const std::vector<Dialect> ALL_DIALECTS{ Dialect::glsl410 };
    return ALL_DIALECTS;
}

#else

const Dialect DEFAULT_DIALECT = Dialect::glsl450;

const std::vector<Dialect> & allDialects() {
    static const std::vector<Dialect> ALL_DIALECTS{ { Dialect::glsl450, Dialect::glsl410 } };
    return ALL_DIALECTS;
}
#endif

const std::vector<Variant>& allVariants() {
    static const std::vector<Variant> ALL_VARIANTS{ { Variant::Mono, Variant::Stereo } };
    return ALL_VARIANTS;
}

const std::string& dialectPath(Dialect dialect) {
    static const std::string e310esPath { "/310es/" };
    static const std::string e410Path { "/410/" };
    static const std::string e450Path { "/450/" };
    switch (dialect) {
#if defined(USE_GLES) 
        case Dialect::glsl310es: return e310esPath;
#else
#if !defined(Q_OS_MAC)
        case Dialect::glsl450: return e450Path;
#endif
        case Dialect::glsl410: return e410Path;
#endif
        default: break;
    }
    throw std::runtime_error("Invalid dialect");
}

static std::string loadResource(const std::string& path) {
    if (!QFileInfo(path.c_str()).exists()) {
        return {};
    }
    return FileUtils::readFile(path.c_str()).toStdString();
}

static Binary loadSpirvResource(const std::string& path) {
    Binary result;
    {
        QFile file(path.c_str());

        if (file.open(QFile::ReadOnly)) {
            QByteArray bytes = file.readAll();
            result.resize(bytes.size());
            memcpy(result.data(), bytes.data(), bytes.size());
        }
    }
    return result;
}

DialectVariantSource loadDialectVariantSource(const std::string& basePath) {
    DialectVariantSource result;
    result.scribe = loadResource(basePath + "scribe");
    result.spirv = loadSpirvResource(basePath + "spirv");
    result.glsl = loadResource(basePath + "glsl");
    String reflectionJson = loadResource(basePath + "json");
    result.reflection.parse(reflectionJson);
    return result;
}

DialectSource loadDialectSource(Dialect dialect, uint32_t shaderId) {
    std::string basePath = std::string(":/shaders/") + std::to_string(shaderId) + dialectPath(dialect);
    DialectSource result;
    result.variantSources[Variant::Mono] = loadDialectVariantSource(basePath);
    auto stereo = loadDialectVariantSource(basePath + "stereo/");
    if (stereo.valid()) {
        result.variantSources[Variant::Stereo] = stereo;
    }
    return result;
}

Source::Pointer Source::loadSource(uint32_t shaderId) {
    auto result = std::make_shared<Source>();
    result->id = shaderId;
    const auto& dialects = allDialects();
    result->name = loadResource(std::string(":/shaders/") + std::to_string(shaderId) + std::string("/name"));
    for (const auto& dialect : dialects) {
        result->dialectSources[dialect] = loadDialectSource(dialect, shaderId);
    }
    return result;
}

Source& Source::operator=(const Source& other) {
    // DO NOT COPY the shader ID
    name = other.name;
    dialectSources = other.dialectSources;
    replacements = other.replacements;
    return *this;
}

const Source& Source::get(uint32_t shaderId) {
    static std::once_flag once;
    static const std::unordered_map<uint32_t, Source::Pointer> shadersById;
    std::call_once(once, [] {
        initShadersResources();
        auto& map = const_cast<std::unordered_map<uint32_t, Source::Pointer>&>(shadersById);
        for (const auto& shaderId : allShaders()) {
            map[shaderId] = loadSource(shaderId);
        }
    });
    const auto itr = shadersById.find(shaderId);
    static const Source EMPTY_SHADER;
    if (itr == shadersById.end()) {
        return EMPTY_SHADER;
    }
    return *(itr->second);
}

bool Source::doReplacement(String& source) const {
    bool replaced = false;
    for (const auto& entry : replacements) {
        const auto& key = entry.first;
        // First try search for a block to replace
        // Blocks are required because oftentimes we need a stub function 
        // in the original source code to allow it to compile.  As such we 
        // need to replace the stub with our own code rather than just inject
        // some code.
        const auto beginMarker = key + "_BEGIN";
        auto beginIndex = source.find(beginMarker);
        if (beginIndex != std::string::npos) {
            const auto endMarker = key + "_END";
            auto endIndex = source.find(endMarker, beginIndex);
            if (endIndex != std::string::npos) {
                auto size = endIndex - beginIndex;
                source.replace(beginIndex, size, entry.second);
                replaced = true;
                continue;
            }
        }

        // If no block is found, try for a simple line replacement
        beginIndex = source.find(key);
        if (beginIndex != std::string::npos) {
            source.replace(beginIndex, key.size(), entry.second);
            replaced = true;
            continue;
        }
    }

    return replaced;
}

const DialectVariantSource& Source::getDialectVariantSource(Dialect dialect, Variant variant) const {
    auto dialectEntry = dialectSources.find(dialect);
    if (dialectEntry == dialectSources.end()) {
        throw std::runtime_error("Dialect source not found");
    }

    const auto& dialectSource = dialectEntry->second;
    auto variantEntry = dialectSource.variantSources.find(variant);
    // FIXME revert to mono if stereo source is requested but not present
    // (for when mono and stereo sources are the same)
    if (variantEntry == dialectSource.variantSources.end()) {
        throw std::runtime_error("Variant source not found");
    }

    return variantEntry->second;
}


String Source::getSource(Dialect dialect, Variant variant) const {
    String result;
    const auto& variantSource = getDialectVariantSource(dialect, variant);
    if (!replacements.empty()) {
        std::string result = variantSource.scribe;
        if (doReplacement(result)) {
            return result;
        }
    }

#if defined(Q_OS_ANDROID) || defined(USE_GLES)
    // SPIRV cross injects "#extension GL_OES_texture_buffer : require" into the GLSL shaders,
    // which breaks android rendering
    return variantSource.scribe;
#else
    if (variantSource.glsl.empty()) {
        return variantSource.scribe;
    }
    return variantSource.glsl;
#endif
}

const Reflection& Source::getReflection(Dialect dialect, Variant variant) const {
    const auto& variantSource = getDialectVariantSource(dialect, variant);
    return variantSource.reflection;
}

static const std::string NAME_KEY{ "name" };
static const std::string INPUTS_KEY{ "inputs" };
static const std::string OUTPUTS_KEY{ "outputs" };
static const std::string UBOS_KEY{ "ubos" };
static const std::string SSBOS_KEY{ "ssbos" };

static const std::string TEXTURES_KEY{ "textures" };
static const std::string LOCATION_KEY{ "location" };
static const std::string BINDING_KEY{ "binding" };
static const std::string TYPE_KEY{ "type" };

std::unordered_set<std::string> populateBufferTextureSet(const nlohmann::json& input) {
    std::unordered_set<std::string> result;
    static const std::string SAMPLER_BUFFER{ "samplerBuffer" };
    auto arraySize = input.size();
    for (size_t i = 0; i < arraySize; ++i) {
        auto entry = input[i];
        std::string name = entry[NAME_KEY];
        std::string type = entry[TYPE_KEY];
        if (type == SAMPLER_BUFFER) {
            result.insert(name);
        }
    }
    return result;
}

Reflection::LocationMap populateLocationMap(const nlohmann::json& input, const std::string& locationKey) {
    Reflection::LocationMap result;
    auto arraySize = input.size();
    for (size_t i = 0; i < arraySize; ++i) {
        auto entry = input[i];
        std::string name = entry[NAME_KEY];
        // Location or binding, depending on the locationKey parameter
        int32_t location = entry[locationKey];
        result[name] = location;
    }
    return result;
}

void Reflection::parse(const std::string& jsonString) {
    if (jsonString.empty()) {
        return;
    }
    using json = nlohmann::json;
    auto root = json::parse(jsonString);

    if (root.count(INPUTS_KEY)) {
        inputs = populateLocationMap(root[INPUTS_KEY], LOCATION_KEY);
    }
    if (root.count(OUTPUTS_KEY)) {
        outputs = populateLocationMap(root[OUTPUTS_KEY], LOCATION_KEY);
    }
    if (root.count(SSBOS_KEY)) {
        resourceBuffers = populateLocationMap(root[SSBOS_KEY], BINDING_KEY);
    }
    if (root.count(UBOS_KEY)) {
        uniformBuffers = populateLocationMap(root[UBOS_KEY], BINDING_KEY);
    }
    if (root.count(TEXTURES_KEY)) {
        textures = populateLocationMap(root[TEXTURES_KEY], BINDING_KEY);
        auto bufferTextures = populateBufferTextureSet(root[TEXTURES_KEY]);
        if (!bufferTextures.empty()) {
            if (!resourceBuffers.empty()) {
                throw std::runtime_error("Input shader has both SSBOs and texture buffers defined");
            }
            for (const auto& bufferTexture : bufferTextures){
                resourceBuffers[bufferTexture] = textures[bufferTexture];
                textures.erase(bufferTexture);
            }
        }
    }
    updateValid();

}


static void mergeMap(Reflection::LocationMap& output, const Reflection::LocationMap& input) {
    for (const auto& entry : input) {
        if (0 != output.count(entry.first)) {
            if (output[entry.first] != entry.second) {
                throw std::runtime_error("Invalid reflection for merging");
            }
        } else {
            output[entry.first] = entry.second;
        }
    }
}

static void updateValidSet(Reflection::ValidSet& output, const Reflection::LocationMap& input) {
    output.clear();
    output.reserve(input.size());
    for (const auto& entry : input) {
        output.insert(entry.second);
    }
}

void Reflection::merge(const Reflection& reflection) {
    mergeMap(textures, reflection.textures);
    mergeMap(uniforms, reflection.uniforms);
    mergeMap(uniformBuffers, reflection.uniformBuffers);
    mergeMap(resourceBuffers, reflection.resourceBuffers);
    updateValid();
}

void Reflection::updateValid() {
    updateValidSet(validInputs, inputs);
    updateValidSet(validOutputs, outputs);
    updateValidSet(validTextures, textures);
    updateValidSet(validUniformBuffers, uniformBuffers);
    updateValidSet(validResourceBuffers, resourceBuffers);
    updateValidSet(validUniforms, uniforms);
}


std::vector<std::string> Reflection::getNames(const LocationMap& locations) {
    std::vector<std::string> result;
    result.reserve(locations.size());
    for (const auto& entry : locations) {
        result.push_back(entry.first);
    }
    return result;
}


}  // namespace shader

