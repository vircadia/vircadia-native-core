//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PlayerWindow.h"

#include <QtCore/QFileInfo>
#include <QtGui/QImageReader>
#include <QtQml/QQmlContext>
#include <QtQuick/QQuickItem>

#include <gpu/Frame.h>
#include <gpu/FrameIO.h>

PlayerWindow::PlayerWindow() {
    setFlags(Qt::MSWindowsOwnDC | Qt::Window | Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint);
    setSurfaceType(QSurface::OpenGLSurface);
    create();
    showFullScreen();

    // Make sure the window has been created by processing events
    QCoreApplication::processEvents();

    // Start the rendering thread
    _renderThread.initialize(this, &_surface);

    // Start the UI
    _surface.resize(size());
    connect(&_surface, &hifi::qml::OffscreenSurface::rootContextCreated, this, [](QQmlContext* context){
        context->setContextProperty("FRAMES_FOLDER", "file:assets:/frames");
    });
    _surface.load("qrc:///qml/main.qml");

    // Connect the UI handler
    QObject::connect(_surface.getRootItem(), SIGNAL(loadFile(QString)),
        this, SLOT(loadFile(QString))
    );

    // Turn on UI input events
    installEventFilter(&_surface);
}

PlayerWindow::~PlayerWindow() {
}

// static const char* FRAME_FILE = "assets:/frames/20190110_1635.json";

static void textureLoader(const std::string& filename, const gpu::TexturePointer& texture, uint16_t layer) {
    QImage image;
    QImageReader(filename.c_str()).read(&image);
    if (layer > 0) {
        return;
    }
    texture->assignStoredMip(0, image.byteCount(), image.constBits());
}

void PlayerWindow::loadFile(QString filename) {
    QString realFilename = QUrl(filename).toLocalFile();
    if (QFileInfo(realFilename).exists()) {
        auto frame = gpu::readFrame(realFilename.toStdString(), _renderThread._externalTexture, &textureLoader);
        _surface.pause();
        _renderThread.submitFrame(frame);
    }
}

void PlayerWindow::touchEvent(QTouchEvent* event) {
    // Super basic input handling when the 3D scene is active.... tap with two finders to return to the
    // QML UI
    static size_t touches = 0;
    switch (event->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
            touches = std::max<size_t>(touches, event->touchPoints().size());
            break;

        case QEvent::TouchEnd:
            if (touches >= 2) {
                _renderThread.submitFrame(nullptr);
                _surface.resume();
            }
            touches = 0;
            break;

        default:
            break;
    }
}
