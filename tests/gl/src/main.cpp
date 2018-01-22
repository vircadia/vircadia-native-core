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


int main(int argc, char** argv) {
    QGuiApplication app(argc, argv);
    
    GLWindow* window = new GLWindow();
    window->create();
    window->show();
    bool contextCreated = false;
    QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [&] {
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

    timer->setInterval(15);
    timer->setSingleShot(false);
    timer->start();
    app.exec();
}
