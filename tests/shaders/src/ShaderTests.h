//
//  Created by Bradley Austin Davis on 2018/06/21
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShaderTests_h
#define hifi_ShaderTests_h

#include <QtTest/QtTest>
#include <gpu/Forward.h>
#include <gl/OffscreenGLCanvas.h>
#include <gl/Context.h>

class ShaderTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testShaderLoad();

private:
    gl::OffscreenContext* _context{ nullptr };
    gpu::ContextPointer _gpuContext;
};

#endif  // hifi_ViewFruxtumTests_h
