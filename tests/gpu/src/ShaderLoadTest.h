//
//  Created by Bradley Austin Davis on 2018/05/08
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <unordered_map>
#include <unordered_set>

#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>

#include <gpu/Forward.h>
#include <gl/OffscreenGLCanvas.h>

#define USE_LOCAL_SHADERS 1

namespace std {
    template <>
    struct hash<std::pair<std::string, std::string>> {
        size_t operator()(const std::pair<std::string, std::string>& a) const {
            std::hash<std::string> hasher;
            return hasher(a.first) + hasher(a.second);
        }
    };

}

using ShadersByName = std::unordered_map<std::string, std::string>;
using Program = std::pair<std::string, std::string>;
using Programs = std::unordered_set<Program>;

class ShaderLoadTest : public QObject {
    Q_OBJECT

private:

    void parseCacheFile();
#if USE_LOCAL_SHADERS
    void parseCacheDirectory();
    void persistCacheDirectory();
#endif
    bool buildProgram(const Program& program);
    void randomizeShaderSources();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testShaderLoad();

private:
    ShadersByName _shaderSources;
    Programs _programs;
    QString _resourcesPath;
    OffscreenGLCanvas _canvas;
    gpu::ContextPointer _gpuContext;
    const glm::uvec2 _size{ 640, 480 };
};
