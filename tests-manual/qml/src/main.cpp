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
#include <QtGui/QOpenGLFunctions_4_1_Core>
#include <QtGui/QOpenGLContext>
#include <QtQuick/QQuickItem>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlComponent>

#include <SettingInterface.h>

#include <gl/OffscreenGLCanvas.h>
#include <PathUtils.h>
#include <NumericalConstants.h>
#include <PerfStat.h>
#include <PathUtils.h>
#include <ViewFrustum.h>
#include <qml/OffscreenSurface.h>
#include <unordered_set>
#include <array>
#include <gl/GLHelpers.h>
#include <gl/Context.h>

#include "TestCase.h"
#include "MacQml.h"

namespace gl {
extern void initModuleGl();
}

class QTestItem : public QQuickItem {
    Q_OBJECT
public:
    QTestItem(QQuickItem* parent = nullptr) : QQuickItem(parent) { qDebug() << __FUNCTION__; }

    ~QTestItem() { qDebug() << __FUNCTION__; }
};

QUrl getTestResource(const QString& relativePath) {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../")) + "/";
        qDebug() << "Resources Path: " << dir;
    }
    return QUrl::fromLocalFile(dir + relativePath);
}

using QmlPtr = QSharedPointer<hifi::qml::OffscreenSurface>;
using TextureAndFence = hifi::qml::OffscreenSurface::TextureAndFence;

class TestWindow : public QWindow {
public:
    TestWindow(const TestCase::Builder& caseBuilder);

private:
    QOpenGLContext _glContext;
    OffscreenGLCanvas _sharedContext;

    TestCase* _testCase{ nullptr };
    QOpenGLFunctions_4_1_Core _glf;
    QSize _size;
    bool _aboutToQuit{ false };
    void initGl();
    void resizeWindow(const QSize& size);
    void draw();
    void resizeEvent(QResizeEvent* ev) override;
};

TestWindow::TestWindow(const TestCase::Builder& builder) {
    Setting::init();
    
    _testCase = builder(this);

    setSurfaceType(QSurface::OpenGLSurface);

    qmlRegisterType<QTestItem>("Hifi", 1, 0, "TestItem");

    show();

    resize(QSize(800, 600));

    auto timer = new QTimer(this);
    timer->setTimerType(Qt::PreciseTimer);
    timer->setInterval(30);
    connect(timer, &QTimer::timeout, [&] { draw(); });
    timer->start();

    connect(qApp, &QCoreApplication::aboutToQuit, [this, timer] {
        timer->stop();
        _aboutToQuit = true;
    });
}

Q_GUI_EXPORT void qt_gl_set_global_share_context(QOpenGLContext *context);
Q_GUI_EXPORT QOpenGLContext *qt_gl_global_share_context();
OffscreenGLCanvas* _chromiumShareContext{ nullptr};
void TestWindow::initGl() {
    _glContext.setFormat(format());
    
    auto globalShareContext = qt_gl_global_share_context();
    if (globalShareContext) {
        _glContext.setShareContext(globalShareContext);
        globalShareContext->makeCurrent(this);
        gl::Context::setupDebugLogging(globalShareContext);
        globalShareContext->doneCurrent();
    }
    
    if (!_glContext.create() || !_glContext.makeCurrent(this)) {
        qFatal("Unable to intialize Window GL context");
    }
    gl::Context::setupDebugLogging(&_glContext);
    gl::initModuleGl();

    _glf.initializeOpenGLFunctions();

    if (!_sharedContext.create(&_glContext) || !_sharedContext.makeCurrent()) {
        qFatal("Unable to intialize Shared GL context");
    }
    hifi::qml::OffscreenSurface::setSharedContext(_sharedContext.getContext());

    if (!globalShareContext) {
        _chromiumShareContext = new OffscreenGLCanvas();
        _chromiumShareContext->setObjectName("ChromiumShareContext");
        _chromiumShareContext->create(&_glContext);
        if (!_chromiumShareContext->makeCurrent()) {
            qFatal("Unable to make chromium shared context current");
        }
        
        qt_gl_set_global_share_context(_chromiumShareContext->getContext());
        _chromiumShareContext->doneCurrent();
    }

    // Restore the GL widget context
    if (!_glContext.makeCurrent(this)) {
        qFatal("Unable to make window context current");
    }

    _testCase->init();
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
    
    _testCase->update();
    
    auto size = geometry().size();
    _glf.glViewport(0, 0, size.width(), size.height());
    _glf.glClearColor(1, 0, 0, 1);
    _glf.glClear(GL_COLOR_BUFFER_BIT);
    
    _testCase->draw();
    
    _glContext.swapBuffers(this);
}

void TestWindow::resizeEvent(QResizeEvent* ev) {
    resizeWindow(ev->size());
}


int main(int argc, char** argv) {
#ifdef Q_OS_MAC
    auto format = getDefaultOpenGLSurfaceFormat();
    // Deal with some weirdness in the chromium context sharing on Mac.
    // The primary share context needs to be 3.2, so that the Chromium will
    // succeed in it's creation of it's command stub contexts.
    format.setVersion(3, 2);
    // This appears to resolve the issues with corrupted fonts on OSX.  No
    // idea why.
    qputenv("QT_ENABLE_GLYPH_CACHE_WORKAROUND", "true");
    // https://i.kym-cdn.com/entries/icons/original/000/008/342/ihave.jpg
    QSurfaceFormat::setDefaultFormat(format);
#endif

    
    QGuiApplication app(argc, argv);
    TestCase::Builder builder = [](QWindow* window)->TestCase*{ return new MacQml(window); };
    TestWindow window(builder);
    return app.exec();
}

#include "main.moc"
