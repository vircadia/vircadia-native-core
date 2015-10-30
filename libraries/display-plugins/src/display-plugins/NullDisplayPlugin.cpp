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

const QString NullDisplayPlugin::NAME("NullDisplayPlugin");

const QString & NullDisplayPlugin::getName() const {
    return NAME;
}

glm::uvec2 NullDisplayPlugin::getRecommendedRenderSize() const {
    return glm::uvec2(100, 100);
}

bool NullDisplayPlugin::hasFocus() const {
    return false;
}

void NullDisplayPlugin::preRender() {}
void NullDisplayPlugin::preDisplay() {}
void NullDisplayPlugin::display(GLuint sceneTexture, const glm::uvec2& sceneSize) {}
void NullDisplayPlugin::finishFrame() {}
void NullDisplayPlugin::stop() {}
