//
//  Created by Bradley Austin Davis on 2018/01/11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextureTest.h"

#include <gpu/Forward.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <gpu/gl/GLBackend.h>
#include <NumericalConstants.h>

QTEST_MAIN(TextureTest)
#pragma optimize("", off)
#define LOAD_TEXTURE_COUNT 40
static const QDir TEST_DIR("D:/ktx_texture_test");

std::string vertexShaderSource = R"SHADER(
#line 14
layout(location = 0) out vec2 outTexCoord0;

const vec4 VERTICES[] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0), 
    vec4( 1.0, -1.0, 0.0, 1.0),    
    vec4(-1.0,  1.0, 0.0, 1.0),
    vec4( 1.0,  1.0, 0.0, 1.0)
);   

void main() {
    outTexCoord0 = VERTICES[gl_VertexID].xy;
    outTexCoord0 += 1.0;
    outTexCoord0 /= 2.0;
    gl_Position = VERTICES[gl_VertexID];
}
)SHADER";

std::string fragmentShaderSource = R"SHADER(
#line 28

uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoord0;
layout(location = 0) out vec4 outFragColor;

void main() {
    outFragColor = texture(tex, inTexCoord0);
    outFragColor.a = 1.0;
    //outFragColor.rb = inTexCoord0;
}

)SHADER";

void TextureTest::initTestCase() {
    getDefaultOpenGLSurfaceFormat();
    _canvas.create();
    if (!_canvas.makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();
    gpu::Texture::setAllowedGPUMemoryUsage(MB_TO_BYTES(4096));

    _canvas.makeCurrent();
    {
        auto VS = gpu::Shader::createVertex(vertexShaderSource);
        auto PS = gpu::Shader::createPixel(fragmentShaderSource);
        auto program = gpu::Shader::createProgram(VS, PS);
        gpu::Shader::BindingSet slotBindings;
        gpu::Shader::makeProgram(*program, slotBindings);
        // If the pipeline did not exist, make it
        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_NONE);
        state->setDepthTest({});
        state->setBlendFunction({ false });
        _pipeline = gpu::Pipeline::create(program, state);
    }

    { _framebuffer.reset(gpu::Framebuffer::create("cached", gpu::Element::COLOR_SRGBA_32, _size.x, _size.y)); }

    {
        auto entryList = TEST_DIR.entryList({ "*.ktx" }, QDir::Filter::Files);
        _textureFiles.reserve(entryList.size());
        for (auto entry : entryList) {
            auto textureFile = TEST_DIR.absoluteFilePath(entry).toStdString();
            _textureFiles.push_back(textureFile);
        }
    }

    {
        std::shuffle(_textureFiles.begin(), _textureFiles.end(), std::default_random_engine());
        size_t newTextureCount = std::min<size_t>(_textureFiles.size(), LOAD_TEXTURE_COUNT);
        for (size_t i = 0; i < newTextureCount; ++i) {
            const auto& textureFile = _textureFiles[i];
            auto texture = gpu::Texture::unserialize(textureFile);
            _textures.push_back(texture);
        }
    }
}

void TextureTest::cleanupTestCase() {
    _gpuContext.reset();
}

void TextureTest::beginFrame() {
    _gpuContext->recycle();
    _gpuContext->beginFrame();
    gpu::doInBatch("TestWindow::beginFrame", _gpuContext, [&](gpu::Batch& batch) {
        batch.setFramebuffer(_framebuffer);
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 0.0f, 0.1f, 0.2f, 1.0f });
        batch.clearDepthFramebuffer(1e4);
        batch.setViewportTransform({ 0, 0, _size.x, _size.y });
    });
}

void TextureTest::endFrame() {
    gpu::doInBatch("TestWindow::endFrame::finish", _gpuContext, [&](gpu::Batch& batch) { batch.resetStages(); });
    auto framePointer = _gpuContext->endFrame();
    _gpuContext->consumeFrameUpdates(framePointer);
    _gpuContext->executeFrame(framePointer);
    // Simulate swapbuffers with a finish
    glFinish();
    QThread::msleep(10);
}

void TextureTest::renderFrame(const std::function<void(gpu::Batch&)>& renderLambda) {
    beginFrame();
    gpu::doInBatch("Test::body", _gpuContext, renderLambda);
    endFrame();
}


inline bool afterUsecs(uint64_t& startUsecs, uint64_t maxIntervalUecs) {
    auto now = usecTimestampNow();
    auto interval = now - startUsecs;
    if (interval > maxIntervalUecs) {
        startUsecs = now;
        return true;
    }
    return false;
}

inline bool afterSecs(uint64_t& startUsecs, uint64_t maxIntervalSecs) {
    return afterUsecs(startUsecs, maxIntervalSecs * USECS_PER_SECOND);
}

template <typename F>
void reportEvery(uint64_t& lastReportUsecs, uint64_t secs, F lamdba) {
    if (afterSecs(lastReportUsecs, secs)) {
        lamdba();
    }
}

inline void failAfter(uint64_t startUsecs, uint64_t secs, const char* message) {
    if (afterSecs(startUsecs, secs)) {
        qFatal(message);
    }
}

void TextureTest::testTextureLoading() {
    auto renderTexturesLamdba = [this](gpu::Batch& batch) {
        batch.setPipeline(_pipeline);
        for (const auto& texture : _textures) {
            batch.setResourceTexture(0, texture);
            batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
        }
    };

    size_t totalSize = 0;
    for (const auto& texture : _textures) {
        totalSize += texture->evalTotalSize();
    }

    auto reportLambda = [=] {
        qDebug() << "Expected" << totalSize;
        qDebug() << "Allowed " << gpu::Texture::getAllowedGPUMemoryUsage();
        qDebug() << "Allocated " << gpu::Context::getTextureResourceGPUMemSize();
        qDebug() << "Populated " << gpu::Context::getTextureResourcePopulatedGPUMemSize();
        qDebug() << "Pending   " << gpu::Context::getTexturePendingGPUTransferMemSize();
    };

    auto lastReport = usecTimestampNow();
    auto start = usecTimestampNow();
    while (totalSize != gpu::Context::getTextureResourceGPUMemSize()) {
        reportEvery(lastReport, 4, reportLambda);
        failAfter(start, 10, "Failed to allocate texture memory after 10 seconds");
        renderFrame(renderTexturesLamdba);
    }

    // Restart the timer
    start = usecTimestampNow();
    auto allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
    auto populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
    while (allocatedMemory != populatedMemory && 0 != gpu::Context::getTexturePendingGPUTransferMemSize()) {
        reportEvery(lastReport, 4, reportLambda);
        failAfter(start, 10, "Failed to populate texture memory after 10 seconds");
        renderFrame();
        allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
        populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
    }
    QCOMPARE(allocatedMemory, totalSize);
    QCOMPARE(populatedMemory, totalSize);
}

