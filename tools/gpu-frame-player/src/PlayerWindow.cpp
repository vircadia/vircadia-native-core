//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PlayerWindow.h"

#include <QtCore/QByteArray>
#include <QtCore/QBuffer>
#include <QtGui/QResizeEvent>
#include <QtGui/QImageReader>
#include <QtGui/QScreen>
#include <QtWidgets/QFileDialog>

#include <gpu/FrameIO.h>

PlayerWindow::PlayerWindow() {
    installEventFilter(this);
    setFlags(Qt::MSWindowsOwnDC | Qt::Window | Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint);

#ifdef USE_GL
    setSurfaceType(QSurface::OpenGLSurface);
#else
    setSurfaceType(QSurface::VulkanSurface);
#endif

    setGeometry(QRect(QPoint(), QSize(800, 600)));
    create();
    show();
    // Ensure the window is visible and the GL context is valid
    QCoreApplication::processEvents();
    _renderThread.initialize(this);
}

PlayerWindow::~PlayerWindow() {
}

bool PlayerWindow::eventFilter(QObject* obj, QEvent* event)  {
    if (event->type() == QEvent::Close) {
        _renderThread.terminate();
    }

    return QWindow::eventFilter(obj, event);
}

void PlayerWindow::loadFrame() {
    static const QString LAST_FILE_KEY{ "lastFile" };
    auto lastScene = _settings.value(LAST_FILE_KEY);
    QString openDir;
    if (lastScene.isValid()) {
        QFileInfo lastSceneInfo(lastScene.toString());
        if (lastSceneInfo.absoluteDir().exists()) {
            openDir = lastSceneInfo.absolutePath();
        }
    }

    QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open File"), openDir, tr("GPU Frames (*.hfb)"));
    if (fileName.isNull()) {
        return;
    }
    _settings.setValue(LAST_FILE_KEY, fileName);
    loadFrame(fileName);
}

void PlayerWindow::keyPressEvent(QKeyEvent* event) {
    bool isShifted = event->modifiers().testFlag(Qt::ShiftModifier);
    float moveScale = isShifted ? 10.0f : 1.0f;
    switch (event->key()) {
        case Qt::Key_F1:
            loadFrame();
            return;

        case Qt::Key_W:
            _renderThread.move(vec3{ 0, 0, -0.1f } * moveScale);
            return;

        case Qt::Key_S:
            _renderThread.move(vec3{ 0, 0, 0.1f } * moveScale);
            return;

        case Qt::Key_A:
            _renderThread.move(vec3{ -0.1f, 0, 0 } * moveScale);
            return;

        case Qt::Key_D:
            _renderThread.move(vec3{ 0.1f, 0, 0 } * moveScale);
            return;

        case Qt::Key_E:
            _renderThread.move(vec3{ 0, 0.1f, 0 } * moveScale);
            return;

        case Qt::Key_F:
            _renderThread.move(vec3{ 0, -0.1f, 0 } * moveScale);
            return;

        default:
            break;
    }
}

void PlayerWindow::resizeEvent(QResizeEvent* ev) {
    _renderThread.resize(ev->size());
}

void PlayerWindow::loadFrame(const QString& path) {
    auto frame = gpu::readFrame(path.toStdString(), _renderThread._externalTexture);
    if (frame) {
        _renderThread.submitFrame(frame);
        if (!_renderThread.isThreaded()) {
            _renderThread.process();
        }
    }
    if (frame->framebuffer) {
        const auto& fbo = *frame->framebuffer;
        glm::uvec2 size{ fbo.getWidth(), fbo.getHeight() };
        
        auto screenSize = screen()->size();
        static const glm::uvec2 maxSize{ screenSize.width() - 100, screenSize.height() - 100 };
        while (glm::any(glm::greaterThan(size, maxSize))) {
            size /= 2;
        }
        resize(size.x, size.y);
    }
}
