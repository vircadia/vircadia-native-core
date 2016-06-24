//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Basic2DWindowOpenGLDisplayPlugin.h"

#include <mutex>

#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QAction>

#include <ui-plugins/PluginContainer.h>

const QString Basic2DWindowOpenGLDisplayPlugin::NAME("Desktop");

static const QString FULLSCREEN = "Fullscreen";

bool Basic2DWindowOpenGLDisplayPlugin::internalActivate() {
    _framerateActions.clear();
    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), FULLSCREEN,
        [this](bool clicked) {
            if (clicked) {
                _container->setFullscreen(getFullscreenTarget());
            } else {
                _container->unsetFullscreen();
            }
        }, true, false);

    return Parent::internalActivate();
}

void Basic2DWindowOpenGLDisplayPlugin::submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) {
    _wantVsync = true; // always
    Parent::submitSceneTexture(frameIndex, sceneTexture);
}

void Basic2DWindowOpenGLDisplayPlugin::internalPresent() {
    if (_wantVsync != isVsyncEnabled()) {
        enableVsync(_wantVsync);
    }
    Parent::internalPresent();
}

static const uint32_t MIN_THROTTLE_CHECK_FRAMES = 60;

bool Basic2DWindowOpenGLDisplayPlugin::isThrottled() const {
    static auto lastCheck = presentCount();
    // Don't access the menu API every single frame
    if ((presentCount() - lastCheck) > MIN_THROTTLE_CHECK_FRAMES) {
        static const QString ThrottleFPSIfNotFocus = "Throttle FPS If Not Focus"; // FIXME - this value duplicated in Menu.h
        _isThrottled  = (!_container->isForeground() && _container->isOptionChecked(ThrottleFPSIfNotFocus));
        lastCheck = presentCount();
    }

    return _isThrottled;
}

// FIXME target the screen the window is currently on
QScreen* Basic2DWindowOpenGLDisplayPlugin::getFullscreenTarget() {
    return qApp->primaryScreen();
}
