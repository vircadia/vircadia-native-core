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
#include <QtCore/QJsonDocument>

#include <GLMHelpers.h>

#include <test-utils/GLMTestUtils.h>
#include <test-utils/QTestExtensions.h>
#include <shared/FileUtils.h>

#include <shaders/Shaders.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <gpu/gl/GLBackend.h>

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
}


QVariantList slotSetToVariantList(const gpu::Shader::SlotSet& slotSet) {
    QVariantList result;
    for (const auto slot : slotSet) {
        QVariantMap inputMap;
        inputMap["name"] = slot._name.c_str();
        inputMap["location"] = slot._location;
        result.append(inputMap);
    }
    return result;
}

std::string reflect(const gpu::ShaderPointer& program) {
    QVariantMap result;
    if (!program->getInputs().empty()) {
        result["inputs"] = slotSetToVariantList(program->getInputs());
    }
    if (!program->getOutputs().empty()) {
        result["outputs"] = slotSetToVariantList(program->getOutputs());
    }
    if (!program->getResourceBuffers().empty()) {
        result["storage_buffers"] = slotSetToVariantList(program->getResourceBuffers());
    }
    if (!program->getSamplers().empty()) {
        result["samplers"] = slotSetToVariantList(program->getSamplers());
    }
    if (!program->getTextures().empty()) {
        result["textures"] = slotSetToVariantList(program->getTextures());
    }
    if (!program->getUniforms().empty()) {
        result["uniforms"] = slotSetToVariantList(program->getUniforms());
    }
    if (!program->getUniformBuffers().empty()) {
        result["uniform_buffers"] = slotSetToVariantList(program->getUniformBuffers());
    }

    return QJsonDocument::fromVariant(result).toJson(QJsonDocument::Indented).toStdString();
}

void ShaderTests::testShaderLoad() {
    size_t index = 0;
    uint32_t INVALID_SHADER_ID = (uint32_t)-1;
    while (INVALID_SHADER_ID != shader::all_programs[index]) {
        auto programId = shader::all_programs[index];
        uint32_t vertexId = programId >> 16;
        uint32_t fragmentId = programId & 0xFF;
        auto vertexSource = shader::loadShaderSource(vertexId);
        QVERIFY(!vertexSource.empty());
        auto fragmentSource = shader::loadShaderSource(fragmentId);
        QVERIFY(!fragmentSource.empty());
        auto program = gpu::Shader::createProgram(programId);
        QVERIFY(gpu::Shader::makeProgram(*program, {}));
        auto reflectionData = reflect(program);
        std::ofstream(std::string("d:/reflection/") + std::to_string(index) + std::string(".json")) << reflectionData;
        ++index;
    }
}
