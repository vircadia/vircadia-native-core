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

void ShaderTests::initTestCase() {
    getDefaultOpenGLSurfaceFormat();
    _canvas.create();
    if (!_canvas.makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();
}

void ShaderTests::cleanupTestCase() {
    qDebug() << "Done";
}

template <typename C, typename F>
QStringList toStringList(const C& c, F f) {
    QStringList result;
    for (const auto& v : c) {
        result << f(v);
    }
    return result;
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

void ShaderTests::testShaderLoad() {
    std::set<uint32_t> usedShaders;
    uint32_t maxShader = 0;
    try {
        size_t index = 0;
        while (shader::INVALID_PROGRAM != shader::all_programs[index]) {
            auto programId = shader::all_programs[index];
            ++index;

            uint32_t vertexId = shader::getVertexId(programId);
            //QVERIFY(0 != vertexId);
            uint32_t fragmentId = shader::getFragmentId(programId);
            QVERIFY(0 != fragmentId);
            usedShaders.insert(vertexId);
            usedShaders.insert(fragmentId);
            maxShader = std::max(maxShader, std::max(fragmentId, vertexId));
            auto vertexSource = gpu::Shader::getShaderSource(vertexId);
            QVERIFY(!vertexSource.getCode().empty());
            auto fragmentSource = gpu::Shader::getShaderSource(fragmentId);
            QVERIFY(!fragmentSource.getCode().empty());

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

#ifdef Q_OS_MAC
                    const auto& uniformRemap = shaderObject.uniformRemap;
#endif
                    auto uniforms = gl::Uniform::load(program);
                    auto expectedUniforms = expectedBindings[gpu::Shader::BindingType::UNIFORM];
                    if (uniforms.size() != expectedUniforms.size()) {
                        qDebug() << "Found" << toStringList(uniforms, [](const auto& v) { return v.name.c_str(); });
                        qDebug() << "Expected" << toStringList(expectedUniforms, [](const auto& v) { return v.first.c_str(); });
                        qDebug() << "Uniforms size mismatch";
                    }
                    for (const auto& uniform : uniforms) {
                        if (0 != expectedUniforms.count(uniform.name)) {
                            auto expectedLocation = expectedUniforms[uniform.name];
#ifdef Q_OS_MAC
                            if (0 != uniformRemap.count(expectedLocation)) {
                                expectedLocation = uniformRemap.at(expectedLocation);
                            }
#endif
                            QVERIFY(expectedLocation == uniform.binding);
                        }
                    }
                }

                // Textures
                {
                    const auto textures = gl::Uniform::loadTextures(program);
                    const auto expectedTextures = expectedBindings[gpu::Shader::BindingType::TEXTURE];
                    if (textures.size() != expectedTextures.size()) {
                        qDebug() << "Found" << toStringList(textures, [](const auto& v) { return v.name.c_str(); });
                        qDebug() << "Expected" << toStringList(expectedTextures, [](const auto& v) { return v.first.c_str(); });
                        qDebug() << "Uniforms size mismatch";
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
                    if (ubos.size() != expectedUbos.size()) {
                        qDebug() << "Found" << toStringList(ubos, [](const auto& v) { return v.name.c_str(); });
                        qDebug() << "Expected" << toStringList(expectedUbos, [](const auto& v) { return v.first.c_str(); });
                        qDebug() << "UBOs size mismatch";
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
