//
//  LocalModelsOverlay.cpp
//  interface/src/ui/overlays
//
//  Created by Ryan Huffman on 07/08/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"

#include "LocalModelsOverlay.h"

LocalModelsOverlay::LocalModelsOverlay(EntityTreeRenderer* entityTreeRenderer) :
    Volume3DOverlay(),
    _entityTreeRenderer(entityTreeRenderer) {
}

LocalModelsOverlay::LocalModelsOverlay(const LocalModelsOverlay* localModelsOverlay) :
    Volume3DOverlay(localModelsOverlay),
    _entityTreeRenderer(localModelsOverlay->_entityTreeRenderer)
{
}

void LocalModelsOverlay::update(float deltatime) {
    _entityTreeRenderer->update();
}

void LocalModelsOverlay::render(RenderArgs* args) {
    if (_visible) {
        float glowLevel = getGlowLevel(); // FIXME, glowing removed for now
        
        auto batch = args ->_batch;
        Application* app = Application::getInstance();
        glm::vec3 oldTranslation = app->getViewMatrixTranslation();
        Transform transform = Transform();
        transform.setTranslation(oldTranslation + getPosition());
        batch->setViewTransform(transform);
        _entityTreeRenderer->render(args);
        transform.setTranslation(oldTranslation);
        batch->setViewTransform(transform);
    }
}

LocalModelsOverlay* LocalModelsOverlay::createClone() const {
    return new LocalModelsOverlay(this);
}
