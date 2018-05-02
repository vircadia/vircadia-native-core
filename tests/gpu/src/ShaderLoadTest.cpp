//
//  Created by Bradley Austin Davis on 2018/01/11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ShaderLoadTest.h"

#include <iostream>
#include <QtCore/QTemporaryFile>

#include <NumericalConstants.h>
#include <gpu/Forward.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <gl/GLShaders.h>

#include <gpu/gl/GLBackend.h>
#include <shared/FileUtils.h>
#include <SettingManager.h>

#include <test-utils/QTestExtensions.h>

//#pragma optimize("", off)

QTEST_MAIN(ShaderLoadTest)

#define FAIL_AFTER_SECONDS 30

QtMessageHandler originalHandler;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
#if defined(Q_OS_WIN)
    OutputDebugStringA(message.toStdString().c_str());
    OutputDebugStringA("\n");
#endif
    originalHandler(type, context, message);
}

void ShaderLoadTest::loadProgramSources() {
    _programSources.clear();
    QString json = FileUtils::readFile(":cache.json");
    auto root = QJsonDocument::fromJson(json.toUtf8()).object();
    _programSources.reserve(root.size());
    QRegularExpression regex("//-------- \\d");
    for (auto shaderKey : root.keys()) {
        auto cacheEntry = root[shaderKey].toObject();
        auto source = cacheEntry["source"].toString();
        auto split = source.split(regex, QString::SplitBehavior::SkipEmptyParts);
        _programSources.emplace_back(split.at(0).trimmed().toStdString(), split.at(1).trimmed().toStdString());
    }
}        


void ShaderLoadTest::initTestCase() {
    originalHandler = qInstallMessageHandler(messageHandler);
    loadProgramSources();
    getDefaultOpenGLSurfaceFormat();
    _canvas.create();
    if (!_canvas.makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();
    _canvas.makeCurrent();
    DependencyManager::set<Setting::Manager>();
}

void ShaderLoadTest::cleanupTestCase() {
    _gpuContext->recycle();
    _gpuContext->shutdown();
    _gpuContext.reset();
    DependencyManager::destroy<Setting::Manager>();
}

std::string randomString() {
    return "\n//" + QUuid::createUuid().toString().toStdString();
}

std::unordered_map<std::string, gpu::ShaderPointer> cachedShaders;

gpu::ShaderPointer getShader(const std::string& shaderSource, bool pixel) {
    if (0 != cachedShaders.count(shaderSource)) {
        return cachedShaders[shaderSource];
    }
    auto shader = pixel ? gpu::Shader::createPixel({ shaderSource + randomString() })  : gpu::Shader::createVertex({ shaderSource + randomString() });
    cachedShaders.insert({shaderSource, shader});
    return shader;
}

void ShaderLoadTest::testShaderLoad() {
    QBENCHMARK {
        for (const auto& programSource : _programSources) {
            auto vertexShader = getShader(programSource.first, false);
            auto pixelShader = getShader(programSource.second, true);
            auto program = gpu::Shader::createProgram(vertexShader, pixelShader);
            QVERIFY(gpu::gl::GLBackend::makeProgram(*program, {}, {}));
        }
    }
}
