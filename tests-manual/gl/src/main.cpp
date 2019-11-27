//
//  Created by Bradley Austin Davis on 2018/01/11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGui/QGuiApplication>
#include <QtCore/QTimer>
#include <gl/Config.h>
#include <gl/GLWindow.h>
#include <gl/GLHelpers.h>


int main(int argc, char** argv) {
    auto glversion = gl::getAvailableVersion();
    auto major = GL_GET_MAJOR_VERSION(glversion);
    auto minor = GL_GET_MINOR_VERSION(glversion);

    if (glversion < GL_MAKE_VERSION(4, 1)) {
        MessageBoxA(nullptr, "Interface requires OpenGL 4.1 or higher", "Unsupported", MB_OK);
        return 0;
    }
    QGuiApplication app(argc, argv);

    bool quitting = false;
    // FIXME need to handle window closing message so that we can stop the timer
    GLWindow* window = new GLWindow();
    window->create();
    window->show();
    window->setSurfaceType(QSurface::OpenGLSurface);
    window->setFormat(getDefaultOpenGLSurfaceFormat());
    bool contextCreated = false;
    QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [&] {
        if (quitting) {
            return;
        }
        if (!contextCreated) {
            window->createContext();
            contextCreated = true;
        }
        if (!window->makeCurrent()) {
            throw std::runtime_error("Failed");
        }
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        window->swapBuffers();
        window->doneCurrent();
    });
    // FIXME need to handle window closing message so that we can stop the timer
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&] {
        quitting = true;
        QObject::disconnect(timer, &QTimer::timeout, nullptr, nullptr);
        timer->stop();
        timer->deleteLater();
    });

    timer->setInterval(15);
    timer->setSingleShot(false);
    timer->start();
    app.exec();
    return 0;
}
