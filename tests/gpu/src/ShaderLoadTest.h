//
//  Created by Bradley Austin Davis on 2018/05/08
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtTest/QtTest>
#include <QtCore/QTemporaryDir>

#include <gpu/Forward.h>
#include <gl/OffscreenGLCanvas.h>

class ShaderLoadTest : public QObject {
    Q_OBJECT

private:
    void loadProgramSources();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testShaderLoad();


private:
    using ProgramSource = std::pair<std::string, std::string>;
    using ProgramSources = std::vector<ProgramSource>;

    ProgramSources _programSources;
    QString _resourcesPath;
    OffscreenGLCanvas _canvas;
    gpu::ContextPointer _gpuContext;
    const glm::uvec2 _size{ 640, 480 };
};
