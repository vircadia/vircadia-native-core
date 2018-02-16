//
//  main.cpp
//  tests/gpu-test/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <unordered_map>
#include <memory>
#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>

#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QOpenGLFunctions_4_5_Core>
#include <QtGui/QOpenGLContext>

#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>

#include <gl/OffscreenGLCanvas.h>
#include <GLMHelpers.h>
#include <PathUtils.h>
#include <NumericalConstants.h>
#include <PerfStat.h>
#include <PathUtils.h>
#include <ViewFrustum.h>
#include <qml/OffscreenSurface.h>


class OffscreenQmlSurface : public hifi::qml::OffscreenSurface {

};

class TestWindow : public QWindow {

public:
    TestWindow();


private:
    using TextureAndFence = hifi::qml::OffscreenSurface::TextureAndFence;
    QOpenGLContext _glContext;
    OffscreenGLCanvas _sharedContext;
    OffscreenQmlSurface _offscreenQml;
    QOpenGLFunctions_4_5_Core _glf;
    uint32_t _currentTexture{ 0 };
    GLsync _readFence{ 0 };
    std::function<void(uint32_t, void*)> _discardLamdba;
    QSize _size;
    GLuint _fbo{ 0 };
    const QSize _qmlSize{ 640, 480 };
    bool _aboutToQuit{ false };
    void initGl();
    void resizeWindow(const QSize& size);
    void draw();
    void resizeEvent(QResizeEvent* ev) override;
};

TestWindow::TestWindow() {
    setSurfaceType(QSurface::OpenGLSurface);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);
    setFormat(format);

    show();

    resize(QSize(800, 600));

    auto timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    timer->setInterval(5);
    connect(timer, &QTimer::timeout, [&] { draw(); });
    timer->start();

    connect(qApp, &QCoreApplication::aboutToQuit, [this, timer] {
        timer->stop();
        _aboutToQuit = true;
    });

}

void TestWindow::initGl() {
    _glContext.setFormat(format());
    if (!_glContext.create() || !_glContext.makeCurrent(this)) {
        qFatal("Unable to intialize Window GL context");
    }

    _glf.initializeOpenGLFunctions();
    _glf.glCreateFramebuffers(1, &_fbo);

    if (!_sharedContext.create(&_glContext) || !_sharedContext.makeCurrent()) {
        qFatal("Unable to intialize Shared GL context");
    }
    hifi::qml::OffscreenSurface::setSharedContext(_sharedContext.getContext());
    _discardLamdba = _offscreenQml.getDiscardLambda();
    _offscreenQml.resize({ 640, 480 });
    _offscreenQml.load(QUrl::fromLocalFile("C:/Users/bdavi/Git/hifi/tests/qml/qml/main.qml"));
}

void TestWindow::resizeWindow(const QSize& size) {
    _size = size;
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

    static std::once_flag once;
    std::call_once(once, [&] { initGl(); });

    if (!_glContext.makeCurrent(this)) {
        return;
    }

    _glf.glClearColor(1, 0, 0, 1);
    _glf.glClear(GL_COLOR_BUFFER_BIT);

    TextureAndFence newTextureAndFence;
    if (_offscreenQml.fetchTexture(newTextureAndFence)) {
        if (_currentTexture) {
            _discardLamdba(_currentTexture, _readFence);
            _readFence = 0;
        }

        _currentTexture = newTextureAndFence.first;
        _glf.glWaitSync((GLsync)newTextureAndFence.second, 0, GL_TIMEOUT_IGNORED);
        _glf.glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _currentTexture, 0);
    }
    
    auto diff = _size - _qmlSize;
    diff /= 2;
    auto qmlExtent = diff + _qmlSize;

    if (_currentTexture) {
        _glf.glBlitNamedFramebuffer(_fbo, 0, 
            0, 0, _qmlSize.width() - 1, _qmlSize.height() - 1, 
            diff.width(), diff.height(), qmlExtent.width() - 1, qmlExtent.height() - 2, 
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    if (_readFence) {
        _glf.glDeleteSync(_readFence);
    }
    _readFence = _glf.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    _glf.glFlush();

    _glContext.swapBuffers(this);
}

void TestWindow::resizeEvent(QResizeEvent* ev) {
    resizeWindow(ev->size());
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!message.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(message.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
    }
}

int main(int argc, char** argv) {   
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
    TestWindow window;
    app.exec();
    return 0;
}

