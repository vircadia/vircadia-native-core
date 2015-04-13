//
//  NullRenderPlugin.cpp
//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "NullRenderPlugin.h"

const QString NullRenderPlugin::NAME("NullRenderPlugin");

const QString & NullRenderPlugin::getName() {
    return NAME;
}

QSize NullRenderPlugin::getRecommendedFramebufferSize() const {
    return QSize(100, 100);
}

glm::ivec2 NullRenderPlugin::getCanvasSize() const {
    return glm::ivec2(100, 100);
}

bool NullRenderPlugin::hasFocus() const {
    return false;
}

glm::ivec2 NullRenderPlugin::getRelativeMousePosition() const {
    return glm::ivec2();
}

glm::ivec2 NullRenderPlugin::getTrueMousePosition() const {
    return glm::ivec2();
}

PickRay NullRenderPlugin::computePickRay(const glm::vec2 & pos) const {
    return PickRay();
}

bool NullRenderPlugin::isMouseOnScreen() {
    return false;
}
