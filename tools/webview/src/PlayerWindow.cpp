//
//  Created by Bradley Austin Davis on 2018/10/21
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PlayerWindow.h"
#include "gl/GLHelpers.h"

PlayerWindow::PlayerWindow() {
    installEventFilter(this);
    setFlags(Qt::MSWindowsOwnDC | Qt::Window | Qt::Dialog | Qt::WindowMinMaxButtonsHint | Qt::WindowTitleHint);
    setSurfaceType(QSurface::OpenGLSurface);
    setFormat(getDefaultOpenGLSurfaceFormat());
    create();
    resize({ 800, 600 });
    show();
    QCoreApplication::processEvents();
    _renderThread.initialize(this);
}

