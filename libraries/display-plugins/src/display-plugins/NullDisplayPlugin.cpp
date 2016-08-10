//
//  NullDisplayPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "NullDisplayPlugin.h"

#include <QtGui/QImage>
#include <ui-plugins/PluginContainer.h>
#include <FramebufferCache.h>
#include <gpu/Frame.h>
#include <gpu/Context.h>

const QString NullDisplayPlugin::NAME("NullDisplayPlugin");

glm::uvec2 NullDisplayPlugin::getRecommendedRenderSize() const {
    return glm::uvec2(100, 100);
}

bool NullDisplayPlugin::hasFocus() const {
    return false;
}

void NullDisplayPlugin::submitFrame(const gpu::FramePointer& frame) {
    if (frame) {
        _gpuContext->consumeFrameUpdates(frame);
    }
}

QImage NullDisplayPlugin::getScreenshot() const {
    return QImage();
}
