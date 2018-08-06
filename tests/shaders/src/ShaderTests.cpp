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

QTEST_MAIN(ShaderTests)

#pragma optimize("", off)
void ShaderTests::initTestCase() {
    _window = new QWindow();
    _context = new ::gl::Context(_window);
    getDefaultOpenGLSurfaceFormat();
    _context->create();
    if (!_context->makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();
}

void ShaderTests::cleanupTestCase() {
    qDebug() << "Done";
}

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

template<typename C>
bool isSubset(const C& parent, const C& child) {
    for (const auto& v : child) {
        if (0 == parent.count(v)) {
            return false;
        }
    }
    return true;
}

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

template <typename K, typename V>
std::unordered_map<V, K> invertMap(const std::unordered_map<K, V>& map) {
    std::unordered_map<V, K> result;
    for (const auto& entry : map) {
        result[entry.second] = entry.first;
    }
    return result;
}

static void verifyBindings(const gpu::Shader::Source& source) {
    const auto reflection = source.getReflection();
    for (const auto& entry : reflection) {
        const auto& map = entry.second;
        const auto reverseMap = invertMap(map);
        if (map.size() != reverseMap.size()) {
            QFAIL("Bindings are not unique");
        }
    }

}


static void verifyInterface(const gpu::Shader::Source& vertexSource, const gpu::Shader::Source& fragmentSource) {
    if (0 == fragmentSource.getReflection().count(gpu::Shader::BindingType::INPUT)) {
        return;
    }
    auto fragIn = fragmentSource.getReflection().at(gpu::Shader::BindingType::INPUT);
    if (0 == vertexSource.getReflection().count(gpu::Shader::BindingType::OUTPUT)) {
        qDebug() << "No vertex output for fragment input";
        //QFAIL("No vertex output for fragment input");
        return;
    }
    auto vout = vertexSource.getReflection().at(gpu::Shader::BindingType::OUTPUT);
    auto vrev = invertMap(vout);
    static const std::string IN_STEREO_SIDE_STRING = "_inStereoSide";
    for (const auto entry : fragIn) {
        const auto& name = entry.first;
        // The presence of "_inStereoSide" in fragment shaders is a bug due to the way we do reflection
        // and use preprocessor macros in the shaders
        if (name == IN_STEREO_SIDE_STRING) {
            continue;
        }
        if (0 == vout.count(name)) {
            qDebug() << "Vertex output missing";
            //QFAIL("Vertex output missing");
            continue;
        }
        const auto& inLocation = entry.second;
        const auto& outLocation = vout.at(name);
        if (inLocation != outLocation) {
            qDebug() << "Mismatch in vertex / fragment interface";
            //QFAIL("Mismatch in vertex / fragment interface");
            continue;
        }
    }
}

template<typename C>
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

void ShaderTests::testShaderLoad() {
    std::set<uint32_t> usedShaders;
    uint32_t maxShader = 0;
    try {

#if 1
        uint32_t testPrograms[] = {
            shader::render_utils::program::parabola,
            shader::INVALID_PROGRAM,
        };
#else
        const auto& testPrograms = shader::all_programs;
#endif

        size_t index = 0;
        while (shader::INVALID_PROGRAM != testPrograms[index]) {
            auto programId = testPrograms[index];
            ++index;

            uint32_t vertexId = shader::getVertexId(programId);
            uint32_t fragmentId = shader::getFragmentId(programId);
            usedShaders.insert(vertexId);
            usedShaders.insert(fragmentId);
            maxShader = std::max(maxShader, std::max(fragmentId, vertexId));
            auto vertexSource = gpu::Shader::getShaderSource(vertexId);
            QVERIFY(!vertexSource.getCode().empty());
            verifyBindings(vertexSource);
            auto fragmentSource = gpu::Shader::getShaderSource(fragmentId);
            QVERIFY(!fragmentSource.getCode().empty());
            verifyBindings(fragmentSource);
            verifyInterface(vertexSource, fragmentSource);

            auto expectedBindings = mergeReflection({ vertexSource, fragmentSource });

            auto program = gpu::Shader::createProgram(programId);
            auto glBackend = std::static_pointer_cast<gpu::gl::GLBackend>(_gpuContext->getBackend());
            auto glshader = gpu::gl::GLShader::sync(*glBackend, *program);
            if (!glshader) {
                qDebug() << "Failed to compile or link vertex " << vertexId << " fragment " << fragmentId;
                continue;
            }

            QVERIFY(glshader != nullptr);
            for (const auto& shaderObject : glshader->_shaderObjects) {
                const auto& program = shaderObject.glprogram;

                // Uniforms
                {
                    auto uniforms = gl::Uniform::load(program);
                    for (const auto& uniform : uniforms) {
                        GLint offset, size;
                        glGetActiveUniformsiv(program, 1, (GLuint*)&uniform.index, GL_UNIFORM_OFFSET, &offset);
                        glGetActiveUniformsiv(program, 1, (GLuint*)&uniform.index, GL_UNIFORM_SIZE, &size);
                        qDebug() << uniform.name.c_str() << " size " << size << "offset" << offset;
                    }
                    const auto& uniformRemap = shaderObject.uniformRemap;
                    auto expectedUniforms = expectedBindings[gpu::Shader::BindingType::UNIFORM];
                    if (!compareBindings(uniforms, expectedUniforms)) {
                        qDebug() << "Uniforms mismatch";
                    }
                    for (const auto& uniform : uniforms) {
                        if (0 != expectedUniforms.count(uniform.name)) {
                            auto expectedLocation = expectedUniforms[uniform.name];
                            if (0 != uniformRemap.count(expectedLocation)) {
                                expectedLocation = uniformRemap.at(expectedLocation);
                            }
                            QVERIFY(expectedLocation == uniform.binding);
                        }
                    }
                }

                // Textures
                {
                    auto textures = gl::Uniform::loadTextures(program);
                    auto expiredBegin = std::remove_if(textures.begin(), textures.end(), [&](const gl::Uniform& uniform) -> bool {
                        return uniform.name == "transformObjectBuffer";
                    });
                    textures.erase(expiredBegin, textures.end());

                    const auto expectedTextures = expectedBindings[gpu::Shader::BindingType::TEXTURE];
                    if (!compareBindings(textures, expectedTextures)) {
                        qDebug() << "Textures mismatch";
                    }
                    for (const auto& texture : textures) {
                        if (0 != expectedTextures.count(texture.name)) {
                            const auto& location = texture.binding;
                            const auto& expectedUnit = expectedTextures.at(texture.name);
                            GLint actualUnit = -1;
                            glGetUniformiv(program, location, &actualUnit);
                            QVERIFY(expectedUnit == actualUnit);
                        }
                    }
                }

                // UBOs
                {
                    auto ubos = gl::UniformBlock::load(program);
                    auto expectedUbos = expectedBindings[gpu::Shader::BindingType::UNIFORM_BUFFER];
                    if (!compareBindings(ubos, expectedUbos)) {
                        qDebug() << "UBOs mismatch";
                    }
                    for (const auto& ubo : ubos) {
                        if (0 != expectedUbos.count(ubo.name)) {
                            QVERIFY(expectedUbos[ubo.name] == ubo.binding);
                        }
                    }
                }

                // FIXME add storage buffer validation
            }
        }
    } catch (const std::runtime_error& error) {
        QFAIL(error.what());
    }

    for (uint32_t i = 0; i <= maxShader; ++i) {
        auto used = usedShaders.count(i);
        if (0 != usedShaders.count(i)) {
            continue;
        }
        auto reflectionJson = shader::loadShaderReflection(i);
        auto name = QJsonDocument::fromJson(reflectionJson.c_str()).object()["name"].toString();
        qDebug() << "Unused shader" << name;
    }

    qDebug() << "Completed all shaders";
}
