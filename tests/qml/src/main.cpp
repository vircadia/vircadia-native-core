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

#define DIVISIONS_X 5
#define DIVISIONS_Y 5

using QmlPtr = QSharedPointer<hifi::qml::OffscreenSurface>;
using TextureAndFence = hifi::qml::OffscreenSurface::TextureAndFence;

struct QmlInfo {
    QmlPtr surface;
    GLuint texture{ 0 };
    uint64_t lifetime{ 0 };
};

class TestWindow : public QWindow {
public:
    TestWindow();

private:
    QOpenGLContext _glContext;
    OffscreenGLCanvas _sharedContext;
    std::array<std::array<QmlInfo, DIVISIONS_Y>, DIVISIONS_X> _surfaces;

    QOpenGLFunctions_4_5_Core _glf;
    std::function<void(uint32_t, void*)> _discardLamdba;
    QSize _size;
    size_t _surfaceCount{ 0 };
    GLuint _fbo{ 0 };
    const QSize _qmlSize{ 640, 480 };
    bool _aboutToQuit{ false };
    uint64_t _createStopTime;
    void initGl();
    void updateSurfaces();
    void buildSurface(QmlInfo& qmlInfo, bool allowVideo);
    void destroySurface(QmlInfo& qmlInfo);
    void resizeWindow(const QSize& size);
    void draw();
    void resizeEvent(QResizeEvent* ev) override;
};

TestWindow::TestWindow() {
    Setting::init();

    setSurfaceType(QSurface::OpenGLSurface);
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);
    setFormat(format);

    qmlRegisterType<QTestItem>("Hifi", 1, 0, "TestItem");

    show();
    _createStopTime = usecTimestampNow() + (3000u * USECS_PER_SECOND);

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

void TestWindow::initGl() {
    _glContext.setFormat(format());
    if (!_glContext.create() || !_glContext.makeCurrent(this)) {
        qFatal("Unable to intialize Window GL context");
    }
    gl::initModuleGl();

    _glf.initializeOpenGLFunctions();
    _glf.glCreateFramebuffers(1, &_fbo);

    if (!_sharedContext.create(&_glContext) || !_sharedContext.makeCurrent()) {
        qFatal("Unable to intialize Shared GL context");
    }
    hifi::qml::OffscreenSurface::setSharedContext(_sharedContext.getContext());
    _discardLamdba = hifi::qml::OffscreenSurface::getDiscardLambda();
}

void TestWindow::resizeWindow(const QSize& size) {
    _size = size;
}

static const int DEFAULT_MAX_FPS = 10;
static const QString CONTROL_URL{ "/qml/controls/WebEntityView.qml" };
static const char* URL_PROPERTY{ "url" };

QString getSourceUrl(bool video) {
    static const std::vector<QString> SOURCE_URLS{
        "https://www.reddit.com/wiki/random",
        "https://en.wikipedia.org/wiki/Wikipedia:Random",
        "https://slashdot.org/",
    };

    static const std::vector<QString> VIDEO_SOURCE_URLS{
        "https://www.youtube.com/watch?v=gDXwhHm4GhM",
        "https://www.youtube.com/watch?v=Ch_hoYPPeGc",
    };

    const auto& sourceUrls = video ? VIDEO_SOURCE_URLS : SOURCE_URLS;
    auto index = rand() % sourceUrls.size();
    return sourceUrls[index];
}

void TestWindow::buildSurface(QmlInfo& qmlInfo, bool video) {
    ++_surfaceCount;
    auto lifetimeSecs = (uint32_t)(5.0f + (randFloat() * 10.0f));
    auto lifetimeUsecs = (USECS_PER_SECOND * lifetimeSecs);
    qmlInfo.lifetime = lifetimeUsecs + usecTimestampNow();
    qmlInfo.texture = 0;
    qmlInfo.surface.reset(new hifi::qml::OffscreenSurface());
    qmlInfo.surface->load(getTestResource(CONTROL_URL), [video](QQmlContext* context, QQuickItem* item) {
        item->setProperty(URL_PROPERTY, getSourceUrl(video));
    });
    qmlInfo.surface->setMaxFps(DEFAULT_MAX_FPS);
    qmlInfo.surface->resize(_qmlSize);
    qmlInfo.surface->resume();
}

void TestWindow::destroySurface(QmlInfo& qmlInfo) {
    auto& surface = qmlInfo.surface;
    auto webView = surface->getRootItem();
    if (webView) {
        // stop loading
        QMetaObject::invokeMethod(webView, "stop");
        webView->setProperty(URL_PROPERTY, "about:blank");
    }
    surface->pause();
    surface.reset();
}

void TestWindow::updateSurfaces() {
    auto now = usecTimestampNow();
    // Fetch any new textures
    for (size_t x = 0; x < DIVISIONS_X; ++x) {
        for (size_t y = 0; y < DIVISIONS_Y; ++y) {
            auto& qmlInfo = _surfaces[x][y];
            if (!qmlInfo.surface) {
                if (now < _createStopTime && randFloat() > 0.99f) {
                    buildSurface(qmlInfo, x == 0 && y == 0);
                } else {
                    continue;
                }
            }

            if (now > qmlInfo.lifetime) {
                destroySurface(qmlInfo);
                continue;
            }

            auto& surface = qmlInfo.surface;
            auto& currentTexture = qmlInfo.texture;

            TextureAndFence newTextureAndFence;
            if (surface->fetchTexture(newTextureAndFence)) {
                if (currentTexture != 0) {
                    auto readFence = _glf.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                    glFlush();
                    _discardLamdba(currentTexture, readFence);
                }
                currentTexture = newTextureAndFence.first;
                _glf.glWaitSync((GLsync)newTextureAndFence.second, 0, GL_TIMEOUT_IGNORED);
            }
        }
    }
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

    updateSurfaces();

    auto size = this->geometry().size();
    auto incrementX = size.width() / DIVISIONS_X;
    auto incrementY = size.height() / DIVISIONS_Y;
    _glf.glViewport(0, 0, size.width(), size.height());
    _glf.glClearColor(1, 0, 0, 1);
    _glf.glClear(GL_COLOR_BUFFER_BIT);
    for (uint32_t x = 0; x < DIVISIONS_X; ++x) {
        for (uint32_t y = 0; y < DIVISIONS_Y; ++y) {
            auto& qmlInfo = _surfaces[x][y];
            if (!qmlInfo.surface || !qmlInfo.texture) {
                continue;
            }
            _glf.glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, qmlInfo.texture, 0);
            _glf.glBlitNamedFramebuffer(_fbo, 0,
                                        // src coordinates
                                        0, 0, _qmlSize.width() - 1, _qmlSize.height() - 1,
                                        // dst coordinates
                                        incrementX * x, incrementY * y, incrementX * (x + 1), incrementY * (y + 1),
                                        // blit mask and filter
                                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
    }
    _glContext.swapBuffers(this);
}

void TestWindow::resizeEvent(QResizeEvent* ev) {
    resizeWindow(ev->size());
}

int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    TestWindow window;
    return app.exec();
}

#include "main.moc"
