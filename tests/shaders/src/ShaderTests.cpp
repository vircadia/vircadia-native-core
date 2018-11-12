//
//  ShaderTests.cpp
//  tests/octree/src
//
//  Created by Andrew Meadows on 2016.02.19
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShaderTests.h"

#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>

#include <QtCore/QJsonDocument>

#include <GLMHelpers.h>

#include <test-utils/GLMTestUtils.h>
#include <test-utils/QTestExtensions.h>
#include <shared/FileUtils.h>

#include <shaders/Shaders.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <gpu/gl/GLBackend.h>
#include <gpu/gl/GLShader.h>
#include <gl/QOpenGLContextWrapper.h>

#define RUNTIME_SHADER_COMPILE_TEST 1

#if RUNTIME_SHADER_COMPILE_TEST
#include <spirv_cross.hpp>
#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/libspirv.hpp>
#include <spirv-tools/optimizer.hpp>
#include <vulkan/vulkan.hpp>

#ifdef DEBUG
#define VCPKG_LIB_DIR "D:/hifi/vcpkg/hifi/installed/x64-windows/debug/lib/"
#else
#define VCPKG_LIB_DIR "D:/hifi/vcpkg/hifi/installed/x64-windows/lib/"
#endif

#pragma comment(lib, VCPKG_LIB_DIR "shaderc_combined.lib")
#pragma comment(lib, VCPKG_LIB_DIR "spirv-cross-core.lib")
#pragma comment(lib, VCPKG_LIB_DIR "spirv-cross-glsl.lib")
#pragma comment(lib, VCPKG_LIB_DIR "spirv-cross-reflect.lib")

#endif

QTEST_MAIN(ShaderTests)

void ShaderTests::initTestCase() {
    _context = new ::gl::OffscreenContext();
    getDefaultOpenGLSurfaceFormat();
    _context->create();
    if (!_context->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    QOpenGLContextWrapper(_context->qglContext()).makeCurrent(_context->getWindow());
    gl::initModuleGl();
    if (!_context->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gpu::Context::init<gpu::gl::GLBackend>();
    if (!_context->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    _gpuContext = std::make_shared<gpu::Context>();
}

void ShaderTests::cleanupTestCase() {
    qDebug() << "Done";
}

#if RUNTIME_SHADER_COMPILE_TEST

template <typename C>
QStringList toQStringList(const C& c) {
    QStringList result;
    for (const auto& v : c) {
        result << v.c_str();
    }
    return result;
}

template <typename C, typename F>
std::unordered_set<std::string> toStringSet(const C& c, F f) {
    std::unordered_set<std::string> result;
    for (const auto& v : c) {
        result.insert(f(v));
    }
    return result;
}

template <typename C>
bool isSubset(const C& parent, const C& child) {
    for (const auto& v : child) {
        if (0 == parent.count(v)) {
            return false;
        }
    }
    return true;
}

#if 0
gpu::Shader::ReflectionMap mergeReflection(const std::initializer_list<const gpu::Shader::Source>& list) {
    gpu::Shader::ReflectionMap result;
    std::unordered_map<gpu::Shader::BindingType, std::unordered_map<uint32_t, std::string>> usedLocationsByType;
    for (const auto& source : list) {
        const auto& reflection = source.getReflection();
        for (const auto& entry : reflection) {
            const auto& type = entry.first;
            if (entry.first == gpu::Shader::BindingType::INPUT || entry.first == gpu::Shader::BindingType::OUTPUT) {
                continue;
            }
            auto& outLocationMap = result[type];
            auto& usedLocations = usedLocationsByType[type];
            const auto& locationMap = entry.second;
            for (const auto& entry : locationMap) {
                const auto& name = entry.first;
                const auto& location = entry.second;
                if (0 != usedLocations.count(location) && usedLocations[location] != name) {
                    qWarning() << QString("Location %1 used by both %2 and %3")
                                      .arg(location)
                                      .arg(name.c_str())
                                      .arg(usedLocations[location].c_str());
                    throw std::runtime_error("Location collision");
                }
                usedLocations[location] = name;
                outLocationMap[name] = location;
            }
        }
    }
    return result;
}
template <typename C>
bool compareBindings(const C& actual, const gpu::Shader::LocationMap& expected) {
    if (actual.size() != expected.size()) {
        auto actualNames = toStringSet(actual, [](const auto& v) { return v.name; });
        auto expectedNames = toStringSet(expected, [](const auto& v) { return v.first; });
        if (!isSubset(expectedNames, actualNames)) {
            qDebug() << "Found" << toQStringList(actualNames);
            qDebug() << "Expected" << toQStringList(expectedNames);
            return false;
        }
    }
    return true;
}

#endif

#endif

template <typename K, typename V>
std::unordered_map<V, K> invertMap(const std::unordered_map<K, V>& map) {
    std::unordered_map<V, K> result;
    for (const auto& entry : map) {
        result[entry.second] = entry.first;
    }
    if (result.size() != map.size()) {
        throw std::runtime_error("Map inversion failure, result size does not match input size");
    }
    return result;
}

static void verifyInterface(const gpu::Shader::Source& vertexSource,
                            const gpu::Shader::Source& fragmentSource,
                            shader::Dialect dialect,
                            shader::Variant variant) {
    const auto& fragmentReflection = fragmentSource.getReflection(dialect, variant);
    if (fragmentReflection.inputs.empty()) {
        return;
    }

    const auto& vertexReflection = vertexSource.getReflection(dialect, variant);
    const auto& fragIn = fragmentReflection.inputs;
    if (vertexReflection.outputs.empty()) {
        throw std::runtime_error("No vertex outputs for fragment inputs");
    }

    const auto& vout = vertexReflection.outputs;
    auto vrev = invertMap(vout);
    for (const auto entry : fragIn) {
        const auto& name = entry.first;
        if (0 == vout.count(name)) {
            throw std::runtime_error("Vertex outputs missing");
        }
        const auto& inLocation = entry.second;
        const auto& outLocation = vout.at(name);
        if (inLocation != outLocation) {
            throw std::runtime_error("Mismatch in vertex / fragment interface");
        }
    }
}

static void verifyInterface(const gpu::Shader::Source& vertexSource, const gpu::Shader::Source& fragmentSource) {
    for (const auto& dialect : shader::allDialects()) {
        for (const auto& variant : shader::allVariants()) {
            verifyInterface(vertexSource, fragmentSource, dialect, variant);
        }
    }
}

#if RUNTIME_SHADER_COMPILE_TEST

void configureGLSLCompilerResources(TBuiltInResource* glslCompilerResources) {
    glslCompilerResources->maxLights = 32;
    glslCompilerResources->maxClipPlanes = 6;
    glslCompilerResources->maxTextureUnits = 32;
    glslCompilerResources->maxTextureCoords = 32;
    glslCompilerResources->maxVertexAttribs = 64;
    glslCompilerResources->maxVertexUniformComponents = 4096;
    glslCompilerResources->maxVaryingFloats = 64;
    glslCompilerResources->maxVertexTextureImageUnits = 32;
    glslCompilerResources->maxCombinedTextureImageUnits = 80;
    glslCompilerResources->maxTextureImageUnits = 32;
    glslCompilerResources->maxFragmentUniformComponents = 4096;
    glslCompilerResources->maxDrawBuffers = 32;
    glslCompilerResources->maxVertexUniformVectors = 128;
    glslCompilerResources->maxVaryingVectors = 8;
    glslCompilerResources->maxFragmentUniformVectors = 16;
    glslCompilerResources->maxVertexOutputVectors = 16;
    glslCompilerResources->maxFragmentInputVectors = 15;
    glslCompilerResources->minProgramTexelOffset = -8;
    glslCompilerResources->maxProgramTexelOffset = 7;
    glslCompilerResources->maxClipDistances = 8;
    glslCompilerResources->maxComputeWorkGroupCountX = 65535;
    glslCompilerResources->maxComputeWorkGroupCountY = 65535;
    glslCompilerResources->maxComputeWorkGroupCountZ = 65535;
    glslCompilerResources->maxComputeWorkGroupSizeX = 1024;
    glslCompilerResources->maxComputeWorkGroupSizeY = 1024;
    glslCompilerResources->maxComputeWorkGroupSizeZ = 64;
    glslCompilerResources->maxComputeUniformComponents = 1024;
    glslCompilerResources->maxComputeTextureImageUnits = 16;
    glslCompilerResources->maxComputeImageUniforms = 8;
    glslCompilerResources->maxComputeAtomicCounters = 8;
    glslCompilerResources->maxComputeAtomicCounterBuffers = 1;
    glslCompilerResources->maxVaryingComponents = 60;
    glslCompilerResources->maxVertexOutputComponents = 64;
    glslCompilerResources->maxGeometryInputComponents = 64;
    glslCompilerResources->maxGeometryOutputComponents = 128;
    glslCompilerResources->maxFragmentInputComponents = 128;
    glslCompilerResources->maxImageUnits = 8;
    glslCompilerResources->maxCombinedImageUnitsAndFragmentOutputs = 8;
    glslCompilerResources->maxCombinedShaderOutputResources = 8;
    glslCompilerResources->maxImageSamples = 0;
    glslCompilerResources->maxVertexImageUniforms = 0;
    glslCompilerResources->maxTessControlImageUniforms = 0;
    glslCompilerResources->maxTessEvaluationImageUniforms = 0;
    glslCompilerResources->maxGeometryImageUniforms = 0;
    glslCompilerResources->maxFragmentImageUniforms = 8;
    glslCompilerResources->maxCombinedImageUniforms = 8;
    glslCompilerResources->maxGeometryTextureImageUnits = 16;
    glslCompilerResources->maxGeometryOutputVertices = 256;
    glslCompilerResources->maxGeometryTotalOutputComponents = 1024;
    glslCompilerResources->maxGeometryUniformComponents = 1024;
    glslCompilerResources->maxGeometryVaryingComponents = 64;
    glslCompilerResources->maxTessControlInputComponents = 128;
    glslCompilerResources->maxTessControlOutputComponents = 128;
    glslCompilerResources->maxTessControlTextureImageUnits = 16;
    glslCompilerResources->maxTessControlUniformComponents = 1024;
    glslCompilerResources->maxTessControlTotalOutputComponents = 4096;
    glslCompilerResources->maxTessEvaluationInputComponents = 128;
    glslCompilerResources->maxTessEvaluationOutputComponents = 128;
    glslCompilerResources->maxTessEvaluationTextureImageUnits = 16;
    glslCompilerResources->maxTessEvaluationUniformComponents = 1024;
    glslCompilerResources->maxTessPatchComponents = 120;
    glslCompilerResources->maxPatchVertices = 32;
    glslCompilerResources->maxTessGenLevel = 64;
    glslCompilerResources->maxViewports = 16;
    glslCompilerResources->maxVertexAtomicCounters = 0;
    glslCompilerResources->maxTessControlAtomicCounters = 0;
    glslCompilerResources->maxTessEvaluationAtomicCounters = 0;
    glslCompilerResources->maxGeometryAtomicCounters = 0;
    glslCompilerResources->maxFragmentAtomicCounters = 8;
    glslCompilerResources->maxCombinedAtomicCounters = 8;
    glslCompilerResources->maxAtomicCounterBindings = 1;
    glslCompilerResources->maxVertexAtomicCounterBuffers = 0;
    glslCompilerResources->maxTessControlAtomicCounterBuffers = 0;
    glslCompilerResources->maxTessEvaluationAtomicCounterBuffers = 0;
    glslCompilerResources->maxGeometryAtomicCounterBuffers = 0;
    glslCompilerResources->maxFragmentAtomicCounterBuffers = 1;
    glslCompilerResources->maxCombinedAtomicCounterBuffers = 1;
    glslCompilerResources->maxAtomicCounterBufferSize = 16384;
    glslCompilerResources->maxTransformFeedbackBuffers = 4;
    glslCompilerResources->maxTransformFeedbackInterleavedComponents = 64;
    glslCompilerResources->maxCullDistances = 8;
    glslCompilerResources->maxCombinedClipAndCullDistances = 8;
    glslCompilerResources->maxSamples = 4;
    glslCompilerResources->limits.nonInductiveForLoops = 1;
    glslCompilerResources->limits.whileLoops = 1;
    glslCompilerResources->limits.doWhileLoops = 1;
    glslCompilerResources->limits.generalUniformIndexing = 1;
    glslCompilerResources->limits.generalAttributeMatrixVectorIndexing = 1;
    glslCompilerResources->limits.generalVaryingIndexing = 1;
    glslCompilerResources->limits.generalSamplerIndexing = 1;
    glslCompilerResources->limits.generalVariableIndexing = 1;
    glslCompilerResources->limits.generalConstantMatrixVectorIndexing = 1;
}

void writeSpirv(const std::string& filename, const std::vector<uint32_t>& spirv) {
    std::ofstream o(filename, std::ios::trunc | std::ios::binary);
    for (const auto& word : spirv) {
        o.write((const char*)&word, sizeof(word));
    }
    o.close();
}

void write(const std::string& filename, const std::string& text) {
    std::ofstream o(filename, std::ios::trunc);
    o.write(text.c_str(), text.size());
    o.close();
}

bool endsWith(const std::string& s, const std::string& f) {
    auto end = s.substr(s.size() - f.size());
    return (end == f);
}

EShLanguage getShaderStage(const std::string& shaderName) {
    static const std::string VERT_EXT{ ".vert" };
    static const std::string FRAG_EXT{ ".frag" };
    static const std::string GEOM_EXT{ ".geom" };
    static const size_t EXT_SIZE = VERT_EXT.size();
    if (shaderName.size() < EXT_SIZE) {
        throw std::runtime_error("Invalid shader name");
    }
    std::string ext = shaderName.substr(shaderName.size() - EXT_SIZE);
    if (ext == VERT_EXT) {
        return EShLangVertex;
    } else if (ext == FRAG_EXT) {
        return EShLangFragment;
    } else if (ext == GEOM_EXT) {
        return EShLangGeometry;
    }
    throw std::runtime_error("Invalid shader name");
}

void rebuildSource(shader::Dialect dialect, shader::Variant variant, const shader::Source& source) {
    try {
        using namespace glslang;
        static const EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
        auto shaderName = source.name;
        const std::string baseOutName = "d:/shaders/" + shaderName;
        auto stage = getShaderStage(shaderName);

        TShader shader(stage);
        std::vector<const char*> strings;
        const auto& dialectVariantSource = source.dialectSources.at(dialect).variantSources.at(variant);
        strings.push_back(dialectVariantSource.scribe.c_str());
        shader.setStrings(strings.data(), (int)strings.size());
        shader.setEnvInput(EShSourceGlsl, stage, EShClientOpenGL, 450);
        shader.setEnvClient(EShClientVulkan, EShTargetVulkan_1_1);
        shader.setEnvTarget(EShTargetSpv, EShTargetSpv_1_3);

        bool success = shader.parse(&glslCompilerResources, 450, false, messages);
        if (!success) {
            qWarning() << "Failed to parse shader " << shaderName.c_str();
            qWarning() << shader.getInfoLog();
            qWarning() << shader.getInfoDebugLog();
            throw std::runtime_error("Wrong");
        }

        // Create and link a shader program containing the single shader
        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(messages)) {
            qWarning() << "Failed to compile shader " << shaderName.c_str();
            qWarning() << program.getInfoLog();
            qWarning() << program.getInfoDebugLog();
            throw std::runtime_error("Wrong");
        }

    // Output the SPIR-V code from the shader program
    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(stage), spirv);

    spvtools::SpirvTools core(SPV_ENV_VULKAN_1_1);
    spvtools::Optimizer opt(SPV_ENV_VULKAN_1_1);

    auto outputLambda = [](spv_message_level_t, const char*, const spv_position_t&, const char* m) { qWarning() << m; };
    core.SetMessageConsumer(outputLambda);
    opt.SetMessageConsumer(outputLambda);

    if (!core.Validate(spirv)) {
        throw std::runtime_error("invalid spirv");
    }
    writeSpirv(baseOutName + ".spv", spirv);

    opt.RegisterPass(spvtools::CreateFreezeSpecConstantValuePass())
        .RegisterPass(spvtools::CreateStrengthReductionPass())
        .RegisterPass(spvtools::CreateEliminateDeadConstantPass())
        .RegisterPass(spvtools::CreateEliminateDeadFunctionsPass())
        .RegisterPass(spvtools::CreateUnifyConstantPass());

    std::vector<uint32_t> optspirv;
    if (!opt.Run(spirv.data(), spirv.size(), &optspirv)) {
        throw std::runtime_error("bad optimize run");
    }
    writeSpirv(baseOutName + ".opt.spv", optspirv);

    std::string disassembly;
    if (!core.Disassemble(optspirv, &disassembly)) {
        throw std::runtime_error("bad disassembly");
    }

        write(baseOutName + ".spv.txt", disassembly);
    } catch (const std::runtime_error& error) {
        qWarning() << error.what();
    }
}
#endif

void validateDialectVariantSource(const shader::DialectVariantSource& source) {
    if (source.scribe.empty()) {
        throw std::runtime_error("Missing scribe source");
    }

    if (source.spirv.empty()) {
        throw std::runtime_error("Missing SPIRV");
    }

    if (source.glsl.empty()) {
        throw std::runtime_error("Missing GLSL");
    }
}

void validaDialectSource(const shader::DialectSource& dialectSource) {
    for (const auto& variant : shader::allVariants()) {
        validateDialectVariantSource(dialectSource.variantSources.find(variant)->second);
    }
}

void validateSource(const shader::Source& shader) {
    if (shader.id == shader::INVALID_SHADER) {
        throw std::runtime_error("Missing stored shader ID");
    }

    if (shader.name.empty()) {
        throw std::runtime_error("Missing shader name");
    }

    static const auto& dialects = shader::allDialects();
    for (const auto dialect : dialects) {
        if (!shader.dialectSources.count(dialect)) {
            throw std::runtime_error("Missing platform shader");
        }
        const auto& platformShader = shader.dialectSources.find(dialect)->second;
        validaDialectSource(platformShader);
    }
}

void ShaderTests::testShaderLoad() {
    try {
        const auto& shaderIds = shader::allShaders();

        // For debugging compile or link failures on individual programs, enable the following block and change the array values
        // Be sure to end with the sentinal value of shader::INVALID_PROGRAM
        const auto& programIds = shader::allPrograms();

        for (auto shaderId : shaderIds) {
            validateSource(shader::Source::get(shaderId));
        }

        {
            std::unordered_set<uint32_t> programUsedShaders;
#pragma omp parallel for
            for (auto programId : programIds) {
                auto vertexId = shader::getVertexId(programId);
                shader::Source::get(vertexId);
                programUsedShaders.insert(vertexId);
                auto fragmentId = shader::getFragmentId(programId);
                shader::Source::get(fragmentId);
                programUsedShaders.insert(fragmentId);
            }

            for (const auto& shaderId : shaderIds) {
                if (programUsedShaders.count(shaderId)) {
                    continue;
                }
                const auto& shader = shader::Source::get(shaderId);
                qDebug() << "Unused shader found" << shader.name.c_str();
            }
        }

        // Traverse all programs again to do program level tests
        for (const auto& programId : programIds) {
            auto vertexId = shader::getVertexId(programId);
            const auto& vertexSource = shader::Source::get(vertexId);
            auto fragmentId = shader::getFragmentId(programId);
            const auto& fragmentSource = shader::Source::get(fragmentId);
            verifyInterface(vertexSource, fragmentSource);

            auto program = gpu::Shader::createProgram(programId);
            auto glBackend = std::static_pointer_cast<gpu::gl::GLBackend>(_gpuContext->getBackend());
            auto glshader = gpu::gl::GLShader::sync(*glBackend, *program);

            if (!glshader) {
                qWarning() << "Failed to compile or link vertex " << vertexSource.name.c_str() << " fragment "
                           << fragmentSource.name.c_str();
                QFAIL("Program link error");
            }
        }
    } catch (const std::runtime_error& error) {
        QFAIL(error.what());
    }

    qDebug() << "Completed all shaders";
}
