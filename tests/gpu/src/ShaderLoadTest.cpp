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
#include <gpu/gl/GLShader.h>
#include <gpu/gl/GLBackend.h>
#include <shared/FileUtils.h>
#include <SettingManager.h>

#include <test-utils/QTestExtensions.h>

#include <test-utils/Utils.h>

QTEST_MAIN(ShaderLoadTest)

extern std::atomic<size_t> gpuBinaryShadersLoaded;

extern const QString& getShaderCacheFile();

std::pair<int, std::vector<std::pair<QString, QString>>> parseCachedShaderString(const QString& cachedShaderString) {

    std::pair<int, std::vector<std::pair<QString, QString>>> result;
    {
        static const QRegularExpression versionRegex("^// VERSION (\\d+)");
        auto match = versionRegex.match(cachedShaderString);
        result.first = match.captured(1).toInt();
    }


    int rangeStart = 0;
    QString type;
    static const QRegularExpression regex("//-------- (\\w+)");
    auto match = regex.match(cachedShaderString, rangeStart);
    while (match.hasMatch()) {
        auto newType = match.captured(1);
        auto start = match.capturedStart(0);
        auto end = match.capturedEnd(0);
        if (rangeStart != 0) {
            QString subString = cachedShaderString.mid(rangeStart, start - rangeStart);
            result.second.emplace_back(type, subString);
        }
        rangeStart = end;
        type = newType;
        match = regex.match(cachedShaderString, rangeStart);
    }

    if (rangeStart != 0) {
        QString subString = cachedShaderString.mid(rangeStart);
        result.second.emplace_back(type, subString);
    }
    return result;
}

std::string getShaderName(const QString& shader) {
    static const QRegularExpression nameExp("//\\s+(\\w+\\.(?:vert|frag))");
    auto match = nameExp.match(shader);
    if (!match.hasMatch()) {
        return (QCryptographicHash::hash(shader.toUtf8(), QCryptographicHash::Md5).toHex() + ".shader").toStdString();
    }
    return match.captured(1).trimmed().toStdString();
}

void ShaderLoadTest::randomizeShaderSources() {
    for (auto& entry : _shaderSources) {
        entry.second += ("\n//" + QUuid::createUuid().toString()).toStdString();
    }
}

#if USE_LOCAL_SHADERS
const QString SHADER_CACHE_FILENAME = "c:/Users/bdavi/AppData/Local/High Fidelity - dev/Interface/shaders/cache.json";
static const QString SHADER_FOLDER = "D:/shaders/";
void ShaderLoadTest::parseCacheDirectory() {
    for (const auto& shaderFile : QDir(SHADER_FOLDER).entryList(QDir::Files)) {
        QString shaderSource = FileUtils::readFile(SHADER_FOLDER + "/" + shaderFile);
        _shaderSources[shaderFile.trimmed().toStdString()] = shaderSource.toStdString();
    }

    auto programsDoc = QJsonDocument::fromJson(FileUtils::readFile(SHADER_FOLDER + "programs.json").toUtf8());
    for (const auto& programElement : programsDoc.array()) {
        auto programObj = programElement.toObject();
        QString vertexSource = programObj["vertex"].toString();
        QString pixelSource = programObj["pixel"].toString();
        _programs.insert({ vertexSource.toStdString(), pixelSource.toStdString() });
    }
}

void ShaderLoadTest::persistCacheDirectory() {
    for (const auto& shaderFile : QDir(SHADER_FOLDER).entryList(QDir::Files)) {
        QFile(SHADER_FOLDER + "/" + shaderFile).remove();
    }

    // Write the shader source files
    for (const auto& entry : _shaderSources) {
        const QString name = entry.first.c_str();
        const QString shader = entry.second.c_str();
        QString fullFile = SHADER_FOLDER + name;
        QVERIFY(!QFileInfo(fullFile).exists());
        QFile shaderFile(fullFile);
        shaderFile.open(QIODevice::WriteOnly);
        shaderFile.write(shader.toUtf8());
        shaderFile.close();
    }

    // Write the list of programs
    {
        QVariantList programsList;
        for (const auto& program : _programs) {
            QVariantMap programMap;
            programMap["vertex"] = program.first.c_str();
            programMap["pixel"] = program.second.c_str();
            programsList.push_back(programMap);
        }

        QFile saveFile(SHADER_FOLDER + "programs.json");
        saveFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate);
        saveFile.write(QJsonDocument::fromVariant(programsList).toJson(QJsonDocument::Indented));
        saveFile.close();
    }
}
#else
const QString SHADER_CACHE_FILENAME = ":cache.json";
#endif

void ShaderLoadTest::parseCacheFile() {
    QString json = FileUtils::readFile(SHADER_CACHE_FILENAME);
    auto root = QJsonDocument::fromJson(json.toUtf8()).object();
    _programs.clear();
    _programs.reserve(root.size());

    const auto keys = root.keys();
    Program program;
    for (auto shaderKey : keys) {
        auto cacheEntry = root[shaderKey].toObject();
        auto source = cacheEntry["source"].toString();
        auto shaders = parseCachedShaderString(source);
        for (const auto& entry : shaders.second) {
            const auto& type = entry.first;
            const auto& source = entry.second;
            const auto name = getShaderName(source);
            if (name.empty()) {
                continue;
            }
            if (0 == _shaderSources.count(name)) {
                _shaderSources[name] = source.toStdString();
            }
            if (type == "vertex") {
                program.first = name;
            } else if (type == "pixel") {
                program.second = name;
            }
        }
        // FIXME support geometry / tesselation shaders eventually
        if (program.first.empty() || program.second.empty()) {
            qFatal("Bad Shader Setup");
        }
        _programs.insert(program);
    }
}        

bool ShaderLoadTest::buildProgram(const Program& programFiles) {
    const auto& vertexName = programFiles.first;
    const auto& vertexSource = _shaderSources[vertexName];
    auto vertexShader = gpu::Shader::createVertex({ vertexSource });

    const auto& pixelName = programFiles.second;
    const auto& pixelSource = _shaderSources[pixelName];
    auto pixelShader = gpu::Shader::createPixel({ pixelSource });

    auto program = gpu::Shader::createProgram(vertexShader, pixelShader);
    return gpu::gl::GLBackend::makeProgram(*program, {}, {});
}

void ShaderLoadTest::initTestCase() {
    installTestMessageHandler();
    DependencyManager::set<Setting::Manager>();
    {
        const auto& shaderCacheFile = getShaderCacheFile();
        if (QFileInfo(shaderCacheFile).exists()) {
            QFile(shaderCacheFile).remove();
        }
    }

    // For local debugging
#if USE_LOCAL_SHADERS
    parseCacheFile();
    persistCacheDirectory();
    parseCacheDirectory();
#else
    parseCacheFile();
#endif

    // We use this to defeat shader caching both by the GPU backend
    // and the OpenGL driver
    randomizeShaderSources();

    QVERIFY(!_programs.empty());
    for (const auto& program : _programs) {
        QVERIFY(_shaderSources.count(program.first) == 1);
        QVERIFY(_shaderSources.count(program.second) == 1);
    }

    getDefaultOpenGLSurfaceFormat();
    _canvas.create();
    if (!_canvas.makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _canvas.makeCurrent();
}

void ShaderLoadTest::cleanupTestCase() {
    DependencyManager::destroy<Setting::Manager>();
}

void ShaderLoadTest::testShaderLoad() {
    _gpuContext = std::make_shared<gpu::Context>();
    QVERIFY(gpuBinaryShadersLoaded == 0);

    auto backend = std::static_pointer_cast<gpu::gl::GLBackend>(_gpuContext->getBackend());
    std::unordered_set<std::string> shaderNames;
    for (const auto& entry : _shaderSources) {
        shaderNames.insert(entry.first);
    }

    QElapsedTimer timer;

    // Initial load of all the shaders
    // No caching
    {
        timer.start();
        for (const auto& program : _programs) {
            QVERIFY(buildProgram(program));
        }
        qDebug() << "Uncached shader load took" << timer.elapsed() << "ms";
        QVERIFY(gpuBinaryShadersLoaded == 0);
    }
    _gpuContext->recycle();
    glFinish();

    // Reload the shaders within the same GPU context lifetime.
    // Shaders will use the cached binaries in memory
    {
        timer.start();
        for (const auto& program : _programs) {
            QVERIFY(buildProgram(program));
        }
        qDebug() << "Cached shader load took" << timer.elapsed() << "ms";
        QVERIFY(gpuBinaryShadersLoaded == _programs.size() * gpu::gl::GLShader::NumVersions);
    }

    // Simulate reloading the shader cache from disk by destroying and recreating the gpu context
    // Shaders will use the cached binaries from disk
    {
        gpuBinaryShadersLoaded = 0;
        _gpuContext->recycle();
        _gpuContext->shutdown();
        _gpuContext.reset();
        _gpuContext = std::make_shared<gpu::Context>();
        _canvas.makeCurrent();
        timer.start();
        for (const auto& program : _programs) {
            QVERIFY(buildProgram(program));
        }
        qDebug() << "Cached shader load took" << timer.elapsed() << "ms";
        QVERIFY(gpuBinaryShadersLoaded == _programs.size() * gpu::gl::GLShader::NumVersions);
    }

}

