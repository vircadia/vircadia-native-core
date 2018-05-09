//
//  Created by Bradley Austin Davis on 2018/05/08
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#include <QtTest/QtTest>
#include <gpu/Forward.h>
#include <gl/OffscreenGLCanvas.h>

class TextureTest : public QObject {
    Q_OBJECT

private:
    void beginFrame();
    void endFrame();
    void renderFrame(const std::function<void(gpu::Batch&)>& = [](gpu::Batch&){});

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testTextureLoading();

private:
    OffscreenGLCanvas _canvas;
    gpu::ContextPointer _gpuContext;
    gpu::PipelinePointer _pipeline;
    gpu::FramebufferPointer _framebuffer;
    gpu::TexturePointer _colorBuffer, _depthBuffer;
    const glm::uvec2 _size{ 640, 480 };
    std::vector<std::string> _textureFiles;
    std::vector<gpu::TexturePointer> _textures;
};
