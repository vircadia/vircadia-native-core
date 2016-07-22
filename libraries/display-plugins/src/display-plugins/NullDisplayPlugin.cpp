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

const QString NullDisplayPlugin::NAME("NullDisplayPlugin");

glm::uvec2 NullDisplayPlugin::getRecommendedRenderSize() const {
    return glm::uvec2(100, 100);
}

bool NullDisplayPlugin::hasFocus() const {
    return false;
}

void NullDisplayPlugin::submitSceneTexture(uint32_t frameIndex, const gpu::TexturePointer& sceneTexture) {
    _container->releaseSceneTexture(sceneTexture);
}

void NullDisplayPlugin::submitOverlayTexture(const gpu::TexturePointer& overlayTexture) {
    _container->releaseOverlayTexture(overlayTexture);
}

QImage NullDisplayPlugin::getScreenshot() const {
    return QImage();
}
