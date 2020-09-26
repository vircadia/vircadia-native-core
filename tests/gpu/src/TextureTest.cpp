//
//  Created by Bradley Austin Davis on 2018/01/11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextureTest.h"

#include <iostream>
#include <QtCore/QTemporaryFile>

#include <gpu/Forward.h>
#include <gl/Config.h>
#include <gl/GLHelpers.h>
#include <gpu/gl/GLBackend.h>
#include <NumericalConstants.h>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include <test-utils/QTestExtensions.h>
#include <test-utils/Utils.h>

#include <ExternalResource.h>

QTEST_MAIN(TextureTest)

#define LOAD_TEXTURE_COUNT 100
#define FAIL_AFTER_SECONDS 30

static const QString TEST_DATA(ExternalResource::getInstance()->getUrl(ExternalResource::Bucket::HF_Public, "/austin/test_data/test_ktx.zip"));
static const QString TEST_DIR_NAME("{630b8f02-52af-4cdf-a896-24e472b94b28}");
static const QString KTX_TEST_DIR_ENV("HIFI_KTX_TEST_DIR");

std::string vertexShaderSource = R"SHADER(
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
uniform sampler2D tex;

layout(location = 0) in vec2 inTexCoord0;
layout(location = 0) out vec4 outFragColor;

void main() {
    outFragColor = texture(tex, inTexCoord0);
    outFragColor.a = 1.0;
    //outFragColor.rb = inTexCoord0;
}

)SHADER";

QtMessageHandler originalHandler;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
#if defined(Q_OS_WIN)
    OutputDebugStringA(message.toStdString().c_str());
    OutputDebugStringA("\n");
#endif
    originalHandler(type, context, message);
}

void TextureTest::initTestCase() {
    originalHandler = qInstallMessageHandler(messageHandler);
    _resourcesPath = getTestResource("interface/resources");
    getDefaultOpenGLSurfaceFormat();
    _canvas.create();
    if (!_canvas.makeCurrent()) {
        qFatal("Unable to make test GL context current");
    }
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _gpuContext = std::make_shared<gpu::Context>();

    if (QProcessEnvironment::systemEnvironment().contains(KTX_TEST_DIR_ENV)) {
        // For local testing with larger data sets
        _resourcesPath = QProcessEnvironment::systemEnvironment().value(KTX_TEST_DIR_ENV);
    } else {
        _resourcesPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + TEST_DIR_NAME;
        if (!QFileInfo(_resourcesPath).exists()) {
            QDir(_resourcesPath).mkpath(".");
            downloadFile(TEST_DATA, [&](const QByteArray& data) {
                QTemporaryFile zipFile;
                if (zipFile.open()) {
                    zipFile.write(data);
                    zipFile.close();
                }
                JlCompress::extractDir(zipFile.fileName(), _resourcesPath);
            });
        }
    }

    QVERIFY(!_resourcesPath.isEmpty());

    _canvas.makeCurrent();
    {
        auto VS = gpu::Shader::createVertex(vertexShaderSource);
        auto PS = gpu::Shader::createPixel(fragmentShaderSource);
        auto program = gpu::Shader::createProgram(VS, PS);
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

    QVERIFY(!_textureFiles.empty());
}

void TextureTest::cleanupTestCase() {
    _framebuffer.reset();
    _pipeline.reset();
    _gpuContext->recycle();
    _gpuContext.reset();
}

std::vector<gpu::TexturePointer> TextureTest::loadTestTextures() const {
    // Load the test textures
    std::vector<gpu::TexturePointer> result;
    size_t newTextureCount = std::min<size_t>(_textureFiles.size(), LOAD_TEXTURE_COUNT);
    for (size_t i = 0; i < newTextureCount; ++i) {
        const auto& textureFile = _textureFiles[i];
        auto texture = gpu::Texture::unserialize(textureFile);
        result.push_back(texture);
    }
    return result;
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
    ++_frameCount;
}
extern QString getTextureMemoryPressureModeString();

void TextureTest::testTextureLoading() {
    QBENCHMARK {
        _frameCount = 0;
        auto textures = loadTestTextures();
        QVERIFY(textures.size() > 0);
        auto renderTexturesLamdba = [&](gpu::Batch& batch) {
            batch.setPipeline(_pipeline);
            for (const auto& texture : textures) {
                batch.setResourceTexture(0, texture);
                batch.draw(gpu::TRIANGLE_STRIP, 4, 0);
            }
        };

        size_t expectedAllocation = 0;
        for (const auto& texture : textures) {
            expectedAllocation += texture->evalTotalSize();
        }
        QVERIFY(textures.size() > 0);

        auto reportLambda = [=] {
            qDebug() << "Allowed   " << gpu::Texture::getAllowedGPUMemoryUsage();
            qDebug() << "Allocated " << gpu::Context::getTextureResourceGPUMemSize();
            qDebug() << "Populated " << gpu::Context::getTextureResourcePopulatedGPUMemSize();
            qDebug() << "Pending   " << gpu::Context::getTexturePendingGPUTransferMemSize();
            qDebug() << "State     " << getTextureMemoryPressureModeString();
        };

        auto allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
        auto populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();

        // Cycle frames we're fully allocated
        // We need to use the texture rendering lambda
        auto lastReport = usecTimestampNow();
        auto start = usecTimestampNow();
        qDebug() << "Awaiting texture allocation";
        while (expectedAllocation != allocatedMemory) {
            doEvery(lastReport, 4, reportLambda);
            failAfter(start, FAIL_AFTER_SECONDS, "Failed to allocate texture memory");
            renderFrame(renderTexturesLamdba);
            allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
            populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
        }
        reportLambda();
        QCOMPARE(allocatedMemory, expectedAllocation);

        // Restart the timer
        start = usecTimestampNow();
        // Cycle frames we're fully populated
        qDebug() << "Awaiting texture population";
        while (allocatedMemory != populatedMemory || 0 != gpu::Context::getTexturePendingGPUTransferMemSize()) {
            doEvery(lastReport, 4, reportLambda);
            failAfter(start, FAIL_AFTER_SECONDS, "Failed to populate texture memory");
            renderFrame();
            allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
            populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
        }
        reportLambda();
        QCOMPARE(populatedMemory, allocatedMemory);
        // FIXME workaround a race condition in the difference between populated size and the actual _populatedMip value in the texture
        for (size_t i = 0; i < textures.size(); ++i) {
            renderFrame();
        }

        // Test on-demand deallocation of memory
        auto maxMemory = allocatedMemory / 2;
        gpu::Texture::setAllowedGPUMemoryUsage(maxMemory);

        // Restart the timer
        start = usecTimestampNow();
        // Cycle frames until the allocated memory is below the max memory
        qDebug() << "Awaiting texture deallocation";
        while (allocatedMemory > maxMemory || allocatedMemory != populatedMemory) {
            doEvery(lastReport, 4, reportLambda);
            failAfter(start, FAIL_AFTER_SECONDS, "Failed to deallocate texture memory");
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
        qDebug() << "Awaiting texture reallocation and repopulation";
        while (allocatedMemory != expectedAllocation || allocatedMemory != populatedMemory) {
            doEvery(lastReport, 4, reportLambda);
            failAfter(start, FAIL_AFTER_SECONDS, "Failed to populate texture memory");
            renderFrame();
            allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
            populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
        }
        reportLambda();
        QCOMPARE(allocatedMemory, expectedAllocation);
        QCOMPARE(populatedMemory, allocatedMemory);

        textures.clear();
        // Cycle frames we're fully populated
        qDebug() << "Awaiting texture deallocation";
        while (allocatedMemory != 0) {
            failAfter(start, FAIL_AFTER_SECONDS, "Failed to clear texture memory");
            renderFrame();
            allocatedMemory = gpu::Context::getTextureResourceGPUMemSize();
            populatedMemory = gpu::Context::getTextureResourcePopulatedGPUMemSize();
        }
        reportLambda();
        QCOMPARE(allocatedMemory, 0);
        QCOMPARE(populatedMemory, 0);
        qDebug() << "Test took " << _frameCount << "frame";
    }
    qDebug() << "Done";
}
