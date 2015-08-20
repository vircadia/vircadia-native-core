//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Basic2DWindowOpenGLDisplayPlugin.h"

#include <plugins/PluginContainer.h>
#include <QWindow>

const QString Basic2DWindowOpenGLDisplayPlugin::NAME("2D Display");

const QString MENU_PARENT = "View";
const QString MENU_NAME = "Display Options";
const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
const QString FULLSCREEN = "Fullscreen";

const QString& Basic2DWindowOpenGLDisplayPlugin::getName() const {
    return NAME;
}

void Basic2DWindowOpenGLDisplayPlugin::activate() {
//    container->addMenu(MENU_PATH);
//    container->addMenuItem(MENU_PATH, FULLSCREEN,
//        [this] (bool clicked) { this->setFullscreen(clicked); },
//        true, false);
    MainWindowOpenGLDisplayPlugin::activate();
}

void Basic2DWindowOpenGLDisplayPlugin::deactivate() {
//    container->removeMenuItem(MENU_NAME, FULLSCREEN);
//    container->removeMenu(MENU_PATH);
    MainWindowOpenGLDisplayPlugin::deactivate();
}

int Basic2DWindowOpenGLDisplayPlugin::getDesiredInterval(bool isThrottled) const {
    static const int THROTTLED_PAINT_TIMER_DELAY = MSECS_PER_SECOND / 15;
    static const int PAINT_TIMER_DELAY_MS = 1;

    return isThrottled ? THROTTLED_PAINT_TIMER_DELAY : PAINT_TIMER_DELAY_MS;
}

bool Basic2DWindowOpenGLDisplayPlugin::isThrottled() const {
    static const QString ThrottleFPSIfNotFocus = "Throttle FPS If Not Focus"; // FIXME - this value duplicated in Menu.h

    bool shouldThrottle = (!CONTAINER->isForeground() && CONTAINER->isOptionChecked(ThrottleFPSIfNotFocus));
    
    if (_isThrottled != shouldThrottle) {
        int desiredInterval = getDesiredInterval(shouldThrottle);
        _timer.start(desiredInterval);
        _isThrottled = shouldThrottle;
    }
    
    return shouldThrottle;
}