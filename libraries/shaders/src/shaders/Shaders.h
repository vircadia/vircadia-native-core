//
//  Created by Bradley Austin Davis on 2018/07/09
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <stdexcept>

#include <QtCore/QtGlobal>

#include <ShaderEnums.h>

namespace shader {

static const uint32_t INVALID_SHADER = (uint32_t)-1;
static const uint32_t INVALID_PROGRAM = (uint32_t)-1;

const std::vector<uint32_t>& allPrograms();
const std::vector<uint32_t>& allShaders();

enum class Dialect
{
#if defined(USE_GLES)
    // GLES only support 3.1 es
    glsl310es,
#elif defined(Q_OS_MAC)
    // Mac only supports 4.1
    glsl410,
#else
    // Everything else supports 4.1 and 4.5
    glsl450,
    glsl410,
#endif
};

extern const Dialect DEFAULT_DIALECT;

const std::vector<Dialect>& allDialects();
const std::string& dialectPath(Dialect dialect);

enum class Variant {
    Mono,
    Stereo,
};

const std::vector<Variant>& allVariants();

static const uint32_t NUM_VARIANTS = 2;

using Binary = std::vector<uint8_t>;
using String = std::string;

struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

struct Reflection {
    using LocationMap = std::unordered_map<std::string, int32_t>;
    using ValidSet = std::unordered_set<int32_t>;

    void parse(const std::string& json);
    void merge(const Reflection& reflection);

    bool validInput(int32_t location) const { return validLocation(validInputs, location); }
    bool validOutput(int32_t location) const { return validLocation(validOutputs, location); }
    bool validTexture(int32_t location) const { return validLocation(validTextures, location); }
    bool validUniform(int32_t location) const { return validLocation(validUniforms, location); }
    bool validUniformBuffer(int32_t location) const { return validLocation(validUniformBuffers, location); }
    bool validResourceBuffer(int32_t location) const { return validLocation(validResourceBuffers, location); }


    LocationMap inputs;

    LocationMap outputs;

    LocationMap textures;

    LocationMap uniformBuffers;

    // Either SSBOs or Textures with the type samplerBuffer, depending on dialect
    LocationMap resourceBuffers;

    // Needed for procedural code, will map to push constants for Vulkan
    LocationMap uniforms;

    static std::vector<std::string> getNames(const LocationMap& locations);

private:

    bool validLocation(const ValidSet& locations, int32_t location) const {
        return locations.count(location) != 0;
    }

    void updateValid();

    ValidSet validInputs;
    ValidSet validOutputs;
    ValidSet validTextures;
    ValidSet validUniformBuffers;
    ValidSet validResourceBuffers;
    ValidSet validUniforms;
};

struct DialectVariantSource {
    // The output of the scribe application with platforms specific headers
    String scribe;
    // Optimized SPIRV version of the shader
    Binary spirv;
    // Regenerated GLSL from the optimized SPIRV
    String glsl;
    // Shader reflection from the optimized SPIRV
    Reflection reflection;

    bool valid() const { return !scribe.empty(); }
};

struct DialectSource {
    std::unordered_map<Variant, DialectVariantSource, EnumClassHash> variantSources;
};

struct Source {
    using Pointer = std::shared_ptr<Source>;
    Source() = default;
    Source& operator=(const Source& other);

    uint32_t id{ INVALID_SHADER };

    // The name of the shader file, with extension, i.e. DrawColor.frag
    std::string name;

    // Map of platforms to their specific shaders
    std::unordered_map<Dialect, DialectSource, EnumClassHash> dialectSources;

    // Support for swapping out code blocks for procedural and debugging shaders
    std::unordered_map<std::string, std::string> replacements;

    String getSource(Dialect dialect, Variant variant) const;
    const Reflection& getReflection(Dialect dialect, Variant variant) const;
    bool valid() const { return !dialectSources.empty(); }
    static Source generate(const std::string& glsl) { throw std::runtime_error("Implement me"); }
    static const Source& get(uint32_t shaderId);

private:
    // Disallow copy construction
    Source(const Source& other) = delete;

    static Source::Pointer loadSource(uint32_t shaderId) ;

    bool doReplacement(String& source) const;
    const DialectVariantSource& getDialectVariantSource(Dialect dialect, Variant variant) const;

};

inline uint32_t getVertexId(uint32_t programId) {
    return (programId >> 16) & UINT16_MAX;
}

inline uint32_t getFragmentId(uint32_t programId) {
    return programId & UINT16_MAX;
}

}  // namespace shader
