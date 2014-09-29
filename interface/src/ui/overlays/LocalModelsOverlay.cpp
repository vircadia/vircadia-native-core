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

LocalModelsOverlay::~LocalModelsOverlay() {
}

void LocalModelsOverlay::update(float deltatime) {
    _entityTreeRenderer->update();
}

void LocalModelsOverlay::render() {
    if (_visible) {

        float glowLevel = getGlowLevel();
        Glower* glower = NULL;
        if (glowLevel > 0.0f) {
            glower = new Glower(glowLevel);
        }

        glPushMatrix(); {
            Application* app = Application::getInstance();
            glm::vec3 oldTranslation = app->getViewMatrixTranslation();
            app->setViewMatrixTranslation(oldTranslation + _position);
            _entityTreeRenderer->render();
            Application::getInstance()->setViewMatrixTranslation(oldTranslation);
        } glPopMatrix();

        if (glower) {
            delete glower;
        }

    }
}
