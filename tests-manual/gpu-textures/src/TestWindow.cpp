//
//  Created by Bradley Austin Davis on 2016/05/16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TestWindow.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QTimer>
#include <QtGui/QResizeEvent>

#include <gl/GLHelpers.h>

#include <gpu/gl/GLBackend.h>

TestWindow::TestWindow() {

    auto timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    timer->setInterval(5);
    connect(timer, &QTimer::timeout, [&] { draw(); });
    timer->start();

    connect(qApp, &QCoreApplication::aboutToQuit, [this, timer] {
        timer->stop();
        _aboutToQuit = true;
    });

    setSurfaceType(QSurface::OpenGLSurface);

    QSurfaceFormat format = getDefaultOpenGLSurfaceFormat();
    format.setOption(QSurfaceFormat::DebugContext);
    setFormat(format);
    _glContext.setFormat(format);
    _glContext.create();
    _glContext.makeCurrent(this);

    show();
}

void TestWindow::initGl() {
    _glContext.makeCurrent(this);
    gl::initModuleGl();
    gpu::Context::init<gpu::gl::GLBackend>();
    _renderArgs->_context = std::make_shared<gpu::Context>();
    _glContext.makeCurrent(this);
    resize(QSize(800, 600));
}

void TestWindow::resizeWindow(const QSize& size) {
    _size = size;
    _renderArgs->_viewport = ivec4(0, 0, _size.width(), _size.height());
}

void TestWindow::beginFrame() {
    _renderArgs->_context->recycle();
    _renderArgs->_context->beginFrame();
    gpu::doInBatch("TestWindow::beginFrame", _renderArgs->_context, [&](gpu::Batch& batch) {
        batch.clearColorFramebuffer(gpu::Framebuffer::BUFFER_COLORS, { 0.0f, 0.1f, 0.2f, 1.0f });
        batch.clearDepthFramebuffer(1e4);
        batch.setViewportTransform({ 0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio() });
    });

    gpu::doInBatch("TestWindow::beginFrame", _renderArgs->_context, [&](gpu::Batch& batch) {
        batch.setViewportTransform(_renderArgs->_viewport);
        batch.setStateScissorRect(_renderArgs->_viewport);
        batch.setProjectionTransform(_projectionMatrix);
    });
}

void TestWindow::endFrame() {
    gpu::doInBatch("TestWindow::endFrame::finish", _renderArgs->_context, [&](gpu::Batch& batch) {
        batch.resetStages();
    });
    auto framePointer = _renderArgs->_context->endFrame();
    _renderArgs->_context->consumeFrameUpdates(framePointer);
    _renderArgs->_context->executeFrame(framePointer);
    _glContext.swapBuffers(this);
}

void TestWindow::draw() {
    if (_aboutToQuit) {
        return;
    }

    // Attempting to draw before we're visible and have a valid size will
    // produce GL errors.
    if (!isVisible() || _size.width() <= 0 || _size.height() <= 0) {
        return;
    }

    if (!_glContext.makeCurrent(this)) {
        return;
    }

    static std::once_flag once;
    std::call_once(once, [&] { initGl(); });
    beginFrame();

    renderFrame();

    endFrame();
}

void TestWindow::resizeEvent(QResizeEvent* ev) {
    resizeWindow(ev->size());
    float fov_degrees = 60.0f;
    float aspect_ratio = (float)_size.width() / _size.height();
    float near_clip = 0.1f;
    float far_clip = 1000.0f;
    _projectionMatrix = glm::perspective(glm::radians(fov_degrees), aspect_ratio, near_clip, far_clip);
}
