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

#include "LocalModelsOverlay.h"

#include <EntityTreeRenderer.h>
#include <gpu/Batch.h>


QString const LocalModelsOverlay::TYPE = "localmodels";

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
        auto batch = args ->_batch;

        Transform transform = Transform();
        transform.setTranslation(args->getViewFrustum().getPosition() + getPosition());
        batch->setViewTransform(transform, true);
        _entityTreeRenderer->render(args);
        transform.setTranslation(args->getViewFrustum().getPosition());
        batch->setViewTransform(transform, true);
    }
}

LocalModelsOverlay* LocalModelsOverlay::createClone() const {
    return new LocalModelsOverlay(this);
}
