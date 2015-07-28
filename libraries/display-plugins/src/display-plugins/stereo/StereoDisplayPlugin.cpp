//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "StereoDisplayPlugin.h"

#include <QApplication>
#include <QDesktopWidget>

#include <GlWindow.h>
#include <ViewFrustum.h>
#include <MatrixStack.h>

#include <gpu/GLBackend.h>

StereoDisplayPlugin::StereoDisplayPlugin() {
}

bool StereoDisplayPlugin::isSupported() const {
    // FIXME this should attempt to do a scan for supported 3D output
    return true;
}

void StereoDisplayPlugin::customizeWindow(PluginContainer * container) {
    _window->setFlags(Qt::FramelessWindowHint);
    auto desktop = QApplication::desktop();
    QRect primaryGeometry = desktop->screenGeometry();
    for (int i = 0; i < desktop->screenCount(); ++i) {
        QRect geometry = desktop->screen(i)->geometry();
        if (geometry.topLeft() == primaryGeometry.topLeft()) {
            continue;
        }
        float aspect = (float)geometry.width() / (float)geometry.height();
        if (aspect < 1.0f) {
            continue;
        }
        _window->setGeometry(geometry);
        break;
    }
    _window->setCursor(Qt::BlankCursor);
    _window->show();
}

// FIXME make this into a setting that can be adjusted
const float DEFAULT_IPD = 0.064f;
const float HALF_DEFAULT_IPD = DEFAULT_IPD / 2.0f;

glm::mat4 StereoDisplayPlugin::getProjection(Eye eye, const glm::mat4& baseProjection) const {
    // Refer to http://www.nvidia.com/content/gtc-2010/pdfs/2010_gtc2010.pdf on creating 
    // stereo projection matrices.  Do NOT use "toe-in", use translation.

    float nearZ = DEFAULT_NEAR_CLIP; // near clipping plane
    float screenZ = 0.25f; // screen projection plane
    // FIXME verify this is the right calculation
    float frustumshift = HALF_DEFAULT_IPD * nearZ / screenZ;
    if (eye == Right) {
        frustumshift = -frustumshift;
    }
    return glm::translate(baseProjection, vec3(frustumshift, 0, 0));
}

glm::mat4 StereoDisplayPlugin::getModelview(Eye eye, const glm::mat4& baseModelview) const {
    float modelviewShift = HALF_DEFAULT_IPD;
    if (eye == Left) {
        modelviewShift = -modelviewShift;
    }
    return baseModelview * glm::translate(mat4(), vec3(modelviewShift, 0, 0));
}

void StereoDisplayPlugin::activate(PluginContainer * container) {
    WindowOpenGLDisplayPlugin::activate(container);
    // FIXME Add menu items
}
