//
//  main.cpp
//  tests/render-utils/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <string>
#include <vector>

#include <gpu/gl/GLBackend.h>

#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLHelpers.h>

#include <QDir>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QResizeEvent>
#include <QTimer>
#include <QWindow>

class RateCounter {
    std::vector<float> times;
    QElapsedTimer timer;
public:
    RateCounter() {
        timer.start();
    }

    void reset() {
        times.clear();
    }

    unsigned int count() const {
        return (unsigned int)times.size() - 1;
    }

    float elapsed() const {
        if (times.size() < 1) {
            return 0.0f;
        }
        float elapsed = *times.rbegin() - *times.begin();
        return elapsed;
    }

    void increment() {
        times.push_back(timer.elapsed() / 1000.0f);
    }

    float rate() const {
        if (elapsed() == 0.0f) {
            return 0.0f;
        }
        return (float) count() / elapsed();
    }
};


const QString& getQmlDir() {
    static QString dir;
    if (dir.isEmpty()) {
        QDir path(__FILE__);
        path.cdUp();
        dir = path.cleanPath(path.absoluteFilePath("../../../interface/resources/qml/")) + "/";
        qDebug() << "Qml Path: " << dir;
    }
    return dir;
}

// Create a simple OpenGL window that renders text in various ways
class QTestWindow : public QWindow {
    Q_OBJECT

    QOpenGLContextWrapper _context;
    QSize _size;
    //TextRenderer* _textRenderer[4];
    RateCounter fps;

protected:
    void renderText();

private:
    void resizeWindow(const QSize& size) {
        _size = size;
    }

public:
    QTestWindow() {
        setSurfaceType(QSurface::OpenGLSurface);

        QSurfaceFormat format;
        // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
        format.setDepthBufferSize(16);
        format.setStencilBufferSize(8);
        setGLFormatVersion(format);
        format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
        format.setOption(QSurfaceFormat::DebugContext);

        setFormat(format);

        _context.setFormat(format);
        _context.create();

        show();
        makeCurrent();

        gpu::Context::init<gpu::gl::GLBackend>();

        qDebug() << (const char*)glGetString(GL_VERSION);

        //_textRenderer[0] = TextRenderer::getInstance(SANS_FONT_FAMILY, 12, false);
        //_textRenderer[1] = TextRenderer::getInstance(SERIF_FONT_FAMILY, 12, false,
        //    TextRenderer::SHADOW_EFFECT);
        //_textRenderer[2] = TextRenderer::getInstance(MONO_FONT_FAMILY, 48, -1,
        //    false, TextRenderer::OUTLINE_EFFECT);
        //_textRenderer[3] = TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, 24);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glDisable(GL_DEPTH_TEST);

        makeCurrent();

        resize(QSize(800, 600));
    }

    virtual ~QTestWindow() {
    }

    void draw();
    void makeCurrent() {
        _context.makeCurrent(this);
    }

protected:

    void resizeEvent(QResizeEvent* ev) override {
        resizeWindow(ev->size());
    }
};

#ifndef SERIF_FONT_FAMILY
#define SERIF_FONT_FAMILY "Times New Roman"
#endif

//static const wchar_t* EXAMPLE_TEXT = L"Hello";
//static const wchar_t* EXAMPLE_TEXT = L"\xC1y Hello 1.0\ny\xC1 line 2\n\xC1y";

void testShaderBuild(const char* vs_src, const char * fs_src) {
    auto vs = gpu::Shader::createVertex(std::string(vs_src));
    auto fs = gpu::Shader::createPixel(std::string(fs_src));
    auto pr = gpu::Shader::createProgram(vs, fs);
    gpu::Shader::makeProgram(*pr);
}

void QTestWindow::draw() {
    if (!isVisible()) {
        return;
    }

    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, _size.width() * devicePixelRatio(), _size.height() * devicePixelRatio());

    _context.swapBuffers(this);
    glFinish();

    fps.increment();
    if (fps.elapsed() >= 2.0f) {
        qDebug() << "FPS: " << fps.rate();
        fps.reset();
    }
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!message.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(message.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#else 
        std::cout << message.toLocal8Bit().constData() << std::endl;
#endif
    }
}


const char * LOG_FILTER_RULES = R"V0G0N(
hifi.gpu=true
)V0G0N";

int main(int argc, char** argv) {    
    QGuiApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);
    QTestWindow window;
    QTimer timer;
    timer.setInterval(1); // Qt::CoarseTimer acceptable
    app.connect(&timer, &QTimer::timeout, &app, [&] {
        window.draw();
    });
    timer.start();
    app.exec();
    return 0;
}

#include "main.moc"
