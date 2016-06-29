//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StereoDisplayPlugin.h"

#include <QtGui/QScreen>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>

#include <ViewFrustum.h>
#include <MatrixStack.h>
#include <ui-plugins/PluginContainer.h>
#include <gl/GLWidget.h>
#include <CursorManager.h>
#include "../CompositorHelper.h"

bool StereoDisplayPlugin::isSupported() const {
    // FIXME this should attempt to do a scan for supported 3D output
    return true;
}

// FIXME make this into a setting that can be adjusted
const float DEFAULT_IPD = 0.064f;

// Default physical display width (50cm)
const float DEFAULT_SCREEN_WIDTH = 0.5f;

// Default separation = ipd / screenWidth
const float DEFAULT_SEPARATION = DEFAULT_IPD / DEFAULT_SCREEN_WIDTH;

// Default convergence depth: where is the screen plane in the virtual space (which depth)
const float DEFAULT_CONVERGENCE = 0.5f;

glm::mat4 StereoDisplayPlugin::getEyeProjection(Eye eye, const glm::mat4& baseProjection) const {
    // Refer to http://www.nvidia.com/content/gtc-2010/pdfs/2010_gtc2010.pdf on creating 
    // stereo projection matrices.  Do NOT use "toe-in", use translation.
    // Updated version: http://developer.download.nvidia.com/assets/gamedev/docs/Siggraph2011-Stereoscopy_From_XY_to_Z-SG.pdf

    float frustumshift = DEFAULT_SEPARATION;
    if (eye == Right) {
        frustumshift = -frustumshift;
    }


    auto eyeProjection = baseProjection;
    eyeProjection[2][0] += frustumshift;
    eyeProjection[3][0] += frustumshift * DEFAULT_CONVERGENCE; // include the eye offset here
    return eyeProjection;
}

static const QString FRAMERATE = DisplayPlugin::MENU_PATH() + ">Framerate";

std::vector<QAction*> _screenActions;
bool StereoDisplayPlugin::internalActivate() {
    auto screens = qApp->screens();
    _screenActions.resize(screens.size());
    for (int i = 0; i < screens.size(); ++i) {
        auto screen = screens.at(i);
        QString name = QString("Screen %1: %2").arg(i + 1).arg(screen->name());
        bool checked = false;
        if (screen == qApp->primaryScreen()) {
            checked = true;
        }
        auto action = _container->addMenuItem(PluginType::DISPLAY_PLUGIN, MENU_PATH(), name,
            [this](bool clicked) { updateScreen(); }, true, checked, "Screens");
        _screenActions[i] = action;
    }

    _container->removeMenu(FRAMERATE);

    _screen = qApp->primaryScreen();
    _container->setFullscreen(_screen);

    return Parent::internalActivate();
}

void StereoDisplayPlugin::updateScreen() {
    for (uint32_t i = 0; i < _screenActions.size(); ++i) {
        if (_screenActions[i]->isChecked()) {
            _screen = qApp->screens().at(i);
            _container->setFullscreen(_screen);
            break;
        }
    }
}

void StereoDisplayPlugin::internalDeactivate() {
    _screenActions.clear();
    _container->unsetFullscreen();
    Parent::internalDeactivate();
}

// Derived classes will override the recommended render size based on the window size,
// so here we want to fix the aspect ratio based on the window, not on the render size
float StereoDisplayPlugin::getRecommendedAspectRatio() const {
    return aspect(Parent::getRecommendedRenderSize());
}

