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

LocalModelsOverlay::LocalModelsOverlay(ModelTreeRenderer* modelTreeRenderer) :
    Volume3DOverlay(),
    _modelTreeRenderer(modelTreeRenderer) {
}

LocalModelsOverlay::~LocalModelsOverlay() {
}

void LocalModelsOverlay::update(float deltatime) {
    _modelTreeRenderer->update();
}

void LocalModelsOverlay::render() {
    if (_visible) {
        glPushMatrix(); {
            Application* app = Application::getInstance();
            glm::vec3 oldTranslation = app->getViewMatrixTranslation();
            app->setViewMatrixTranslation(oldTranslation + _position);

            _modelTreeRenderer->render();

            Application::getInstance()->setViewMatrixTranslation(oldTranslation);
        } glPopMatrix();
    }
}
