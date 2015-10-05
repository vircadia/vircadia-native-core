//
//  ModelRenderPayload.cpp
//  interface/src/renderer
//
//  Created by Sam Gateau on 10/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelRenderPayload.h"

#include "Model.h"

using namespace render;

MeshPartPayload::MeshPartPayload(Model* model, int meshIndex, int partIndex, int shapeIndex) :
    model(model), url(model->getURL()), meshIndex(meshIndex), partIndex(partIndex), _shapeID(shapeIndex)
{
}

render::ItemKey MeshPartPayload::getKey() const {
    if (!model->isVisible()) {
        return ItemKey::Builder().withInvisible().build();
    }
    auto geometry = model->getGeometry();
    if (!geometry.isNull()) {
        auto drawMaterial = geometry->getShapeMaterial(_shapeID);
        if (drawMaterial) {
            auto matKey = drawMaterial->_material->getKey();
            if (matKey.isTransparent() || matKey.isTransparentMap()) {
                return ItemKey::Builder::transparentShape();
            } else {
                return ItemKey::Builder::opaqueShape();
            }
        }
    }
    
    // Return opaque for lack of a better idea
    return ItemKey::Builder::opaqueShape();
}

render::Item::Bound MeshPartPayload::getBound() const {
    if (_isBoundInvalid) {
        model->getPartBounds(meshIndex, partIndex);
        _isBoundInvalid = false;
    }
    return _bound;
}

void MeshPartPayload::render(RenderArgs* args) const {
    return model->renderPart(args, meshIndex, partIndex, _shapeID);
}

