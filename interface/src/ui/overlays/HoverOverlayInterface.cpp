//
//  HoverOverlayInterface.cpp
//  interface/src/ui/overlays
//
//  Created by Zach Fox on 2017-07-14.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HoverOverlayInterface.h"
#include "Application.h"
#include <EntityTreeRenderer.h>

HoverOverlayInterface::HoverOverlayInterface() {
    // "hover_overlay" debug log category disabled by default.
    // Create your own "qtlogging.ini" file and set your "QT_LOGGING_CONF" environment variable
    // if you'd like to enable/disable certain categories.
    // More details: http://doc.qt.io/qt-5/qloggingcategory.html#configuring-categories
    QLoggingCategory::setFilterRules(QStringLiteral("hifi.hover_overlay.debug=false"));

    _entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    _entityPropertyFlags += PROP_POSITION;
    _entityPropertyFlags += PROP_ROTATION;

    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>().data();
    connect(entityTreeRenderer, SIGNAL(hoverEnterEntity(const EntityItemID&, const PointerEvent&)), this, SLOT(createHoverOverlay(const EntityItemID&, const PointerEvent&)));
    connect(entityTreeRenderer, SIGNAL(hoverLeaveEntity(const EntityItemID&, const PointerEvent&)), this, SLOT(destroyHoverOverlay(const EntityItemID&, const PointerEvent&)));
}

void HoverOverlayInterface::createHoverOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    qCDebug(hover_overlay) << "Creating Hover Overlay on top of entity with ID: " << entityItemID;
    setCurrentHoveredEntity(entityItemID);

    EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);

    if (_hoverOverlayID == UNKNOWN_OVERLAY_ID || !qApp->getOverlays().isAddedOverlay(_hoverOverlayID)) {
        _hoverOverlay = std::make_shared<Image3DOverlay>();
        _hoverOverlay->setAlpha(1.0f);
        _hoverOverlay->setPulseMin(0.75f);
        _hoverOverlay->setPulseMax(1.0f);
        _hoverOverlay->setColorPulse(1.0f);
        _hoverOverlay->setIgnoreRayIntersection(false);
        _hoverOverlay->setDrawInFront(true);
        _hoverOverlay->setURL("http://i.imgur.com/gksZygp.png");
        _hoverOverlay->setIsFacingAvatar(true);
        _hoverOverlay->setDimensions(glm::vec2(0.2f, 0.2f) * glm::distance(entityProperties.getPosition(), qApp->getCamera().getPosition()));
        _hoverOverlayID = qApp->getOverlays().addOverlay(_hoverOverlay);
    }

    _hoverOverlay->setPosition(entityProperties.getPosition());
    _hoverOverlay->setRotation(entityProperties.getRotation());
    _hoverOverlay->setVisible(true);
}

void HoverOverlayInterface::destroyHoverOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    qCDebug(hover_overlay) << "Destroying Hover Overlay on top of entity with ID: " << entityItemID;
    setCurrentHoveredEntity(QUuid());

    qApp->getOverlays().deleteOverlay(_hoverOverlayID);
    _hoverOverlay = NULL;
    _hoverOverlayID = UNKNOWN_OVERLAY_ID;
}

void HoverOverlayInterface::destroyHoverOverlay(const EntityItemID& entityItemID) {
    HoverOverlayInterface::destroyHoverOverlay(entityItemID, PointerEvent());
}

void HoverOverlayInterface::clickHoverOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (overlayID == _hoverOverlayID) {
        qCDebug(hover_overlay) << "Clicked Hover Overlay. Entity ID:" << _currentHoveredEntity << "Overlay ID:" << overlayID;
    }
}
