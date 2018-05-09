//
//  Created by Bradley Austin Davis on 2018/01/11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextureTest.h"

#include <QtCore/QTemporaryFile>

#include <gpu/Forward.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <gpu/gl/GLBackend.h>
#include <NumericalConstants.h>
#include <test-utils/FileDownloader.h>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include "../../QTestExtensions.h"

#pragma optimize("", off)

QTEST_MAIN(TextureTest)

#define LOAD_TEXTURE_COUNT 40

static const QString TEST_DATA("https://hifi-public.s3.amazonaws.com/austin/test_data/test_ktx.zip");

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

#define USE_SERVER_DATA 1

void TextureTest::initTestCase() {
    _resourcesPath = getTestResource("interface/resources");
    getDefaultOpenGLSurfaceFormat();
    _canvas.create();
    if (!_canvas.makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();
#if USE_SERVER_DATA
    if (!_testDataDir.isValid()) {
        qFatal("Unable to create temp directory");
    }

    QString path = _testDataDir.path();
    FileDownloader(TEST_DATA,
                   [&](const QByteArray& data) {
                       QTemporaryFile zipFile;
                       if (zipFile.open()) {
                           zipFile.write(data);
                           zipFile.close();
                       }
                       if (!_testDataDir.isValid()) {
                           qFatal("Unable to create temp dir");
                       }
                       auto files = JlCompress::extractDir(zipFile.fileName(), _testDataDir.path());
                       for (const auto& file : files) {
                           qDebug() << file;
                       }
                   })
        .waitForDownload();
    _resourcesPath = _testDataDir.path();
#else
    _resourcesPath = "D:/test_ktx";
#endif

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

    _framebuffer.reset(gpu::Framebuffer::create("cached", gpu::Element::COLOR_SRGBA_32, _size.x, _size.y));

    // Find the test textures
    {
        QDir resourcesDir(_resourcesPath);
        auto entryList = resourcesDir.entryList({ "*.ktx" }, QDir::Filter::Files);
        _textureFiles.reserve(entryList.size());
        for (auto entry : entryList) {
            auto textureFile = resourcesDir.absoluteFilePath(entry).toStdString();
            _textureFiles.push_back(textureFile);
        }
    }

    // Load the test textures
    {
        size_t newTextureCount = std::min<size_t>(_textureFiles.size(), LOAD_TEXTURE_COUNT);
        for (size_t i = 0; i < newTextureCount; ++i) {
            const auto& textureFile = _textureFiles[i];
            auto texture = gpu::Texture::unserialize(textureFile);
            _textures.push_back(texture);
        }
    }
}

void TextureTest::cleanupTestCase() {
    _framebuffer.reset();
    _pipeline.reset();
    _gpuContext->recycle();
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

void TextureTest::testTextureLoading() {
    QVERIFY(_textures.size() > 0);
    auto renderTexturesLamdba = [this](gpu::Batch& batch) {
        batch.setPipeline(_pipeline);
        for (const auto& texture : _textures) {
            batch.setResourceTexture(0, texture);
            batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
        }
    };

    size_t expectedAllocation = 0;
    for (const auto& texture : _textures) {
        expectedAllocation += texture->evalTotalSize();
    }
    QVERIFY(_textures.size() > 0);

    auto reportLambda = [=] {
        qDebug() << "Allowed   " << gpu::Texture::getAllowedGPUMemoryUsage();
        qDebug() << "Allocated " << gpu::Context::getTextureResourceGPUMemSize();
        qDebug() << "Populated " << gpu::Context::getTextureResourcePopulatedGPUMemSize();
        qDebug() << "Pending   " << gpu::Context::getTexturePendingGPUTransferMemSize();
    };

    auto allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
    auto populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();

    // Cycle frames we're fully allocated
    // We need to use the texture rendering lambda
    auto lastReport = usecTimestampNow();
    auto start = usecTimestampNow();
    while (expectedAllocation != allocatedMemory) {
        reportEvery(lastReport, 4, reportLambda);
        failAfter(start, 10, "Failed to allocate texture memory after 10 seconds");
        renderFrame(renderTexturesLamdba);
        allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
        populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
    }
    QCOMPARE(allocatedMemory, expectedAllocation);

    // Restart the timer
    start = usecTimestampNow();
    // Cycle frames we're fully populated
    while (allocatedMemory != populatedMemory || 0 != gpu::Context::getTexturePendingGPUTransferMemSize()) {
        reportEvery(lastReport, 4, reportLambda);
        failAfter(start, 10, "Failed to populate texture memory after 10 seconds");
        renderFrame();
        allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
        populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
    }
    reportLambda();
    QCOMPARE(populatedMemory, allocatedMemory);

    // FIXME workaround a race condition in the difference between populated size and the actual _populatedMip value in the texture
    for (size_t i = 0; i < _textures.size(); ++i) {
        renderFrame();
    }

    // Test on-demand deallocation of memory
    auto maxMemory = allocatedMemory / 2;
    gpu::Texture::setAllowedGPUMemoryUsage(maxMemory);

    // Restart the timer
    start = usecTimestampNow();
    // Cycle frames until the allocated memory is below the max memory
    while (allocatedMemory > maxMemory || allocatedMemory != populatedMemory) {
        reportEvery(lastReport, 4, reportLambda);
        failAfter(start, 10, "Failed to deallocate texture memory after 10 seconds");
        renderFrame(renderTexturesLamdba);
        allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
        populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
    }
    reportLambda();

    // Verify that the allocation is now below the target
    QVERIFY(allocatedMemory <= maxMemory);
    // Verify that populated memory is the same as allocated memory
    QCOMPARE(populatedMemory, allocatedMemory);

    // Restart the timer
    start = usecTimestampNow();
    // Reset the max memory to automatic
    gpu::Texture::setAllowedGPUMemoryUsage(0);
    // Cycle frames we're fully populated
    while (allocatedMemory != expectedAllocation || allocatedMemory != populatedMemory) {
        reportEvery(lastReport, 4, reportLambda);
        failAfter(start, 10, "Failed to populate texture memory after 10 seconds");
        renderFrame();
        allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
        populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
    }
    reportLambda();
    QCOMPARE(allocatedMemory, expectedAllocation);
    QCOMPARE(populatedMemory, allocatedMemory);
}
