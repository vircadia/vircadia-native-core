//
//  main.cpp
//  tests/render-utils/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextRenderer.h"
#include "MatrixStack.h"

#include <QWindow>
#include <QFile>
#include <QTime>
#include <QImage>
#include <QTimer>
#include <QElapsedTimer>
#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QResizeEvent>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QApplication>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <PathUtils.h>

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
        return times.size() - 1;
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
            return NAN;
        }
        return (float) count() / elapsed();
    }
};

// Create a simple OpenGL window that renders text in various ways
class QTestWindow: public QWindow {
    Q_OBJECT
    QOpenGLContext * _context;
    QSize _size;
    TextRenderer* _textRenderer[4];
    RateCounter fps;

protected:
    void resizeEvent(QResizeEvent * ev) override {
        QWindow::resizeEvent(ev);
        _size = ev->size();
        resizeGl();
    }

    void resizeGl() {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, _size.width(), _size.height(), 0, 1, -1);
        glMatrixMode(GL_MODELVIEW);
        glViewport(0, 0, _size.width(), _size.height());
    }

public:
    QTestWindow();
    virtual ~QTestWindow() {

    }
    void makeCurrent() {
        _context->makeCurrent(this);
    }

    void draw();
};

#ifndef SERIF_FONT_FAMILY
#define SERIF_FONT_FAMILY "Times New Roman"
#endif

QTestWindow::QTestWindow() {
    setSurfaceType(QSurface::OpenGLSurface);

    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    format.setProfile(
            QSurfaceFormat::OpenGLContextProfile::CompatibilityProfile);
    setFormat(format);

    _context = new QOpenGLContext;
    _context->setFormat(format);
    _context->create();

    show();
    makeCurrent();
    qDebug() << (const char*) glGetString(GL_VERSION);

#ifdef WIN32
    glewExperimental = true;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        const GLubyte * errStr = glewGetErrorString(err);
        qDebug("Error: %s\n", errStr);
    }
    qDebug("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    if (wglewGetExtension("WGL_EXT_swap_control")) {
        int swapInterval = wglGetSwapIntervalEXT();
        qDebug("V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    }
    glGetError();
#endif

    setFramePosition(QPoint(100, -900));
    resize(QSize(800, 600));
    _size = QSize(800, 600);

    _textRenderer[0] = TextRenderer::getInstance(SANS_FONT_FAMILY, 12, false);
    _textRenderer[1] = TextRenderer::getInstance(SERIF_FONT_FAMILY, 12, false,
            TextRenderer::SHADOW_EFFECT);
    _textRenderer[2] = TextRenderer::getInstance(MONO_FONT_FAMILY, 48, -1,
            false, TextRenderer::OUTLINE_EFFECT);
    _textRenderer[3] = TextRenderer::getInstance(INCONSOLATA_FONT_FAMILY, 24);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.2f, 0.2f, 1);
    glDisable(GL_DEPTH_TEST);
    resizeGl();
}

static const wchar_t * EXAMPLE_TEXT = L"Hello";
//static const wchar_t * EXAMPLE_TEXT = L"\xC1y Hello 1.0\ny\xC1 line 2\n\xC1y";
static const glm::uvec2 QUAD_OFFSET(10, 10);

static const glm::vec3 COLORS[4] = { { 1.0, 1.0, 1.0 }, { 0.5, 1.0, 0.5 }, {
        1.0, 0.5, 0.5 }, { 0.5, 0.5, 1.0 } };

void QTestWindow::draw() {
    makeCurrent();
    glClear(GL_COLOR_BUFFER_BIT);

    const glm::uvec2 size = glm::uvec2(_size.width() / 2, _size.height() / 2);
    const glm::uvec2 offsets[4] = { { QUAD_OFFSET.x, QUAD_OFFSET.y }, { size.x
        + QUAD_OFFSET.x, QUAD_OFFSET.y }, { size.x + QUAD_OFFSET.x, size.y
        + QUAD_OFFSET.y }, { QUAD_OFFSET.x, size.y + QUAD_OFFSET.y }, };

    QString str = QString::fromWCharArray(EXAMPLE_TEXT);
    for (int i = 0; i < 4; ++i) {
        glm::vec2 bounds = _textRenderer[i]->computeExtent(str);
        glPushMatrix();
        {
            glTranslatef(offsets[i].x, offsets[i].y, 0);
            glColor3f(0, 0, 0);
            glBegin(GL_QUADS);
            {
                glVertex2f(0, 0);
                glVertex2f(0, bounds.y);
                glVertex2f(bounds.x, bounds.y);
                glVertex2f(bounds.x, 0);
            }
            glEnd();
        }
        glPopMatrix();
        const int testCount = 100;
        for (int j = 0; j < testCount; ++j) {
            // Draw backgrounds around where the text will appear
            // Draw the text itself
            _textRenderer[i]->draw(offsets[i].x, offsets[i].y, str.toLocal8Bit().constData(),
                glm::vec4(COLORS[i], 1.0f));
        }
    }
    _context->swapBuffers(this);
    glFinish();
    fps.increment();
    if (fps.elapsed() >= 2.0f) {
        qDebug() << "FPS: " << fps.rate();
        fps.reset();
    }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QTestWindow window;
    QTimer timer;
    timer.setInterval(1);
    app.connect(&timer, &QTimer::timeout, &app, [&] {
        window.draw();
    });
    timer.start();
    app.exec();
    return 0;
}

#include "main.moc"
