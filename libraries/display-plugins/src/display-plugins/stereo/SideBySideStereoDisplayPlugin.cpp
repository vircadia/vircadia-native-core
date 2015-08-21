//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SideBySideStereoDisplayPlugin.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>

#include <GlWindow.h>
#include <ViewFrustum.h>
#include <MatrixStack.h>

#include <gpu/GLBackend.h>
#include <plugins/PluginContainer.h>

const QString SideBySideStereoDisplayPlugin::NAME("3D TV - Side by Side Stereo");

static const QString MENU_PATH = "Display";
static const QString FULLSCREEN = "Fullscreen";

const QString & SideBySideStereoDisplayPlugin::getName() const {
    return NAME;
}

SideBySideStereoDisplayPlugin::SideBySideStereoDisplayPlugin() {
}

void SideBySideStereoDisplayPlugin::activate() {
    CONTAINER->addMenu(MENU_PATH);
    CONTAINER->addMenuItem(MENU_PATH, FULLSCREEN,
        [this](bool clicked) {
            if (clicked) {
                CONTAINER->setFullscreen(getFullscreenTarget());
            } else {
                CONTAINER->unsetFullscreen();
            }
        }, true, false);
    StereoDisplayPlugin::activate();
}

// FIXME target the screen the window is currently on
QScreen* SideBySideStereoDisplayPlugin::getFullscreenTarget() {
    return qApp->primaryScreen();
}
