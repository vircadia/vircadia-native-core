//
//  Overlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Overlay.h"

#include <NumericalConstants.h>
#include <RegisteredMetaTypes.h>

#include "Application.h"

Overlay::Overlay() :
    _renderItemID(render::Item::INVALID_ITEM_ID)
{
}

Overlay::Overlay(const Overlay* overlay) :
    _renderItemID(render::Item::INVALID_ITEM_ID),
    _visible(overlay->_visible)
{
}

void Overlay::setProperties(const QVariantMap& properties) {
    if (properties["visible"].isValid()) {
        bool visible = properties["visible"].toBool();
        setVisible(visible);
    }
}

bool Overlay::addToScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    _renderItemID = scene->allocateID();
    transaction.resetItem(_renderItemID, std::make_shared<Overlay::Payload>(overlay));
    return true;
}

void Overlay::removeFromScene(Overlay::Pointer overlay, const render::ScenePointer& scene, render::Transaction& transaction) {
    transaction.removeItem(_renderItemID);
    render::Item::clearID(_renderItemID);
}

render::ItemKey Overlay::getKey() {
    auto builder = render::ItemKey::Builder().withTypeShape().withTypeMeta();

    builder.withViewSpace();
    builder.withLayer(render::hifi::LAYER_2D);

    if (!_visible) {
        builder.withInvisible();
    }

    render::hifi::Tag viewTagBits = render::hifi::TAG_ALL_VIEWS;
    builder.withTagBits(viewTagBits);

    return builder.build();
}