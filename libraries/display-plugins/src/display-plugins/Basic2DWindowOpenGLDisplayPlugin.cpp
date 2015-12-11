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

#include <plugins/PluginContainer.h>

const QString Basic2DWindowOpenGLDisplayPlugin::NAME("2D Display");

static const QString FULLSCREEN = "Fullscreen";
static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";
static const QString FRAMERATE_UNLIMITED = "Unlimited";
static const QString FRAMERATE_60 = "60";
static const QString FRAMERATE_50 = "50";
static const QString FRAMERATE_40 = "40";
static const QString FRAMERATE_30 = "30";
static const QString VSYNC_ON = "V-Sync On";

const QString& Basic2DWindowOpenGLDisplayPlugin::getName() const {
    return NAME;
}

void Basic2DWindowOpenGLDisplayPlugin::activate() {
    WindowOpenGLDisplayPlugin::activate();

    _framerateActions.clear();
    _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), FULLSCREEN,
        [this](bool clicked) {
            if (clicked) {
                _container->setFullscreen(getFullscreenTarget());
            } else {
                _container->unsetFullscreen();
            }
        }, true, false);
    _container->addMenu(FRAMERATE);
    _framerateActions.push_back(
        _container->addMenuItem(PluginType::DISPLAY_PLUGIN, FRAMERATE, FRAMERATE_UNLIMITED,
            [this](bool) { updateFramerate(); }, true, true, FRAMERATE));
    _framerateActions.push_back(
        _container->addMenuItem(PluginType::DISPLAY_PLUGIN, FRAMERATE, FRAMERATE_60,
            [this](bool) { updateFramerate(); }, true, false, FRAMERATE));
    _framerateActions.push_back(
        _container->addMenuItem(PluginType::DISPLAY_PLUGIN, FRAMERATE, FRAMERATE_50,
            [this](bool) { updateFramerate(); }, true, false, FRAMERATE));
    _framerateActions.push_back(
        _container->addMenuItem(PluginType::DISPLAY_PLUGIN, FRAMERATE, FRAMERATE_40,
            [this](bool) { updateFramerate(); }, true, false, FRAMERATE));
    _framerateActions.push_back(
        _container->addMenuItem(PluginType::DISPLAY_PLUGIN, FRAMERATE, FRAMERATE_30,
            [this](bool) { updateFramerate(); }, true, false, FRAMERATE));

    // Vsync detection happens in the parent class activate, so we need to check after that
    if (_vsyncSupported) {
        _vsyncAction = _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), VSYNC_ON, [this](bool) {}, true, true);
    } else {
        _vsyncAction = nullptr;
    }

    updateFramerate();
}

void Basic2DWindowOpenGLDisplayPlugin::submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) {
    if (_vsyncAction) {
        _wantVsync = _vsyncAction->isChecked();
    }

    WindowOpenGLDisplayPlugin::submitSceneTexture(frameIndex, sceneTexture, sceneSize);
}

void Basic2DWindowOpenGLDisplayPlugin::internalPresent() {
    if (_wantVsync != isVsyncEnabled()) {
        enableVsync(_wantVsync);
    }
    WindowOpenGLDisplayPlugin::internalPresent();
}
const uint32_t THROTTLED_FRAMERATE = 15;
int Basic2DWindowOpenGLDisplayPlugin::getDesiredInterval() const {
    static const int ULIMIITED_PAINT_TIMER_DELAY_MS = 1;
    int result = ULIMIITED_PAINT_TIMER_DELAY_MS;
    if (0 != _framerateTarget) {
        result = MSECS_PER_SECOND / _framerateTarget;
    } else if (_isThrottled) {
        // This test wouldn't be necessary if we could depend on updateFramerate setting _framerateTarget.
        // Alas, that gets complicated: isThrottled() is const and other stuff depends on it.
        result = MSECS_PER_SECOND / THROTTLED_FRAMERATE;
    }

    return result;
}

bool Basic2DWindowOpenGLDisplayPlugin::isThrottled() const {
    static const QString ThrottleFPSIfNotFocus = "Throttle FPS If Not Focus"; // FIXME - this value duplicated in Menu.h

    bool shouldThrottle = (!_container->isForeground() && _container->isOptionChecked(ThrottleFPSIfNotFocus));
    
    if (_isThrottled != shouldThrottle) {
        _isThrottled = shouldThrottle;
        _timer.start(getDesiredInterval());
    }
    
    return shouldThrottle;
}

void Basic2DWindowOpenGLDisplayPlugin::updateFramerate() {
    QAction* checkedFramerate{ nullptr };
    foreach(auto action, _framerateActions) {
        if (action->isChecked()) {
            checkedFramerate = action;
            break;
        }
    }

    _framerateTarget = 0;
    if (checkedFramerate) {
        QString actionText = checkedFramerate->text();
        if (FRAMERATE_60 == actionText) {
            _framerateTarget = 60;
        } else if (FRAMERATE_50 == actionText) {
            _framerateTarget = 50;
        } else if (FRAMERATE_40 == actionText) {
            _framerateTarget = 40;
        } else if (FRAMERATE_30 == actionText) {
            _framerateTarget = 30;
        }
    } else if (_isThrottled) {
        _framerateTarget = THROTTLED_FRAMERATE;
    }

    int newInterval = getDesiredInterval();
    _timer.start(newInterval);
}

// FIXME target the screen the window is currently on
QScreen* Basic2DWindowOpenGLDisplayPlugin::getFullscreenTarget() {
    return qApp->primaryScreen();
}
