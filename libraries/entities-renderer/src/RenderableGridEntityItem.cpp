//
//  Created by Sam Gondelman on 11/29/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RenderableGridEntityItem.h"

using namespace render;
using namespace render::entities;

GridEntityRenderer::GridEntityRenderer(const EntityItemPointer& entity) : Parent(entity) {
}

bool GridEntityRenderer::needsRenderUpdate() const {
    return Parent::needsRenderUpdate();
}

bool GridEntityRenderer::needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const {
    return false;
}

void GridEntityRenderer::doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) {

}

ItemKey GridEntityRenderer::getKey() {
    ItemKey::Builder builder;
    builder.withTypeShape().withTypeMeta().withTagBits(getTagMask());

    withReadLock([&] {
        if (isTransparent()) {
            builder.withTransparent();
        } else if (_canCastShadow) {
            builder.withShadowCaster();
        }
    });

    return builder.build();
}

ShapeKey GridEntityRenderer::getShapeKey() {
    ShapeKey::Builder builder;
    return builder.build();
}

void GridEntityRenderer::doRender(RenderArgs* args) {

}