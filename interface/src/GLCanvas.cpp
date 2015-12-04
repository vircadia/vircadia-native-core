//
//  GLCanvas.cpp
//  interface/src
//
//  Created by Stephen Birarda on 8/14/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// FIXME ordering of headers
#include "Application.h"
#include "GLCanvas.h"

#include <QWindow>

#include "MainWindow.h"
#include "Menu.h"

void GLCanvas::paintGL() {
    PROFILE_RANGE(__FUNCTION__);
    
    // FIXME - I'm not sure why this still remains, it appears as if this GLCanvas gets a single paintGL call near
    // the beginning of the application starting up. I'm not sure if we really need to call Application::paintGL()
    // in this case, since the display plugins eventually handle all the painting
    bool isThrottleFPSEnabled = Menu::getInstance()->isOptionChecked(MenuOption::ThrottleFPSIfNotFocus);
    if (!qApp->getWindow()->isMinimized() || !isThrottleFPSEnabled) {
        qApp->paintGL();
    }
}

void GLCanvas::resizeGL(int width, int height) {
    qApp->resizeGL();
}

bool GLCanvas::event(QEvent* event) {
    if (QEvent::Paint == event->type() && qApp->isAboutToQuit()) {
        return true;
    }
    return GLWidget::event(event);
}
