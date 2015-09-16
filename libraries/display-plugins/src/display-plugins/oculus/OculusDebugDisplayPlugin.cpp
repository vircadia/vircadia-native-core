//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OculusDebugDisplayPlugin.h"
#include <QtCore/QProcessEnvironment>

const QString OculusDebugDisplayPlugin::NAME("Oculus Rift (Simulator)");

const QString & OculusDebugDisplayPlugin::getName() const {
    return NAME;
}

static const QString DEBUG_FLAG("HIFI_DEBUG_OCULUS");
static bool enableDebugOculus = QProcessEnvironment::systemEnvironment().contains("HIFI_DEBUG_OCULUS");

bool OculusDebugDisplayPlugin::isSupported() const {
    if (!enableDebugOculus) {
        return false;
    }
    return OculusBaseDisplayPlugin::isSupported();
}

void OculusDebugDisplayPlugin::customizeContext() {
    WindowOpenGLDisplayPlugin::customizeContext();
    enableVsync(false);
}

void OculusDebugDisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    WindowOpenGLDisplayPlugin::display(finalTexture, sceneSize);
    OculusBaseDisplayPlugin::display(finalTexture, sceneSize);
}

void OculusDebugDisplayPlugin::finishFrame() {
    swapBuffers();
    doneCurrent();
};
