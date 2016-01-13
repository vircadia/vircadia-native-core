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
#include <plugins/PluginContainer.h>

const QString NullDisplayPlugin::NAME("NullDisplayPlugin");

glm::uvec2 NullDisplayPlugin::getRecommendedRenderSize() const {
    return glm::uvec2(100, 100);
}

bool NullDisplayPlugin::hasFocus() const {
    return false;
}

void NullDisplayPlugin::submitSceneTexture(uint32_t frameIndex, uint32_t sceneTexture, const glm::uvec2& sceneSize) {
    _container->releaseSceneTexture(sceneTexture);
}

void NullDisplayPlugin::submitOverlayTexture(uint32_t overlayTexture, const glm::uvec2& overlaySize) {
    _container->releaseOverlayTexture(overlayTexture);
}

void NullDisplayPlugin::stop() {}

QImage NullDisplayPlugin::getScreenshot() const {
    return QImage();
}
