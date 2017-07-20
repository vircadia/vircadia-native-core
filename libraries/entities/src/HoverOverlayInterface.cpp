//
//  HoverOverlayInterface.cpp
//  libraries/entities/src
//
//  Created by Zach Fox on 2017-07-14.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HoverOverlayInterface.h"

HoverOverlayInterface::HoverOverlayInterface() {
    // "hover_overlay" debug log category disabled by default.
    // Create your own "qtlogging.ini" file and set your "QT_LOGGING_CONF" environment variable
    // if you'd like to enable/disable certain categories.
    // More details: http://doc.qt.io/qt-5/qloggingcategory.html#configuring-categories
    QLoggingCategory::setFilterRules(QStringLiteral("hifi.hover_overlay.debug=false"));
}

void HoverOverlayInterface::createHoverOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    qCDebug(hover_overlay) << "Creating Hover Overlay on top of entity with ID: " << entityItemID;
    setCurrentHoveredEntity(entityItemID);
}

void HoverOverlayInterface::createHoverOverlay(const EntityItemID& entityItemID) {
    HoverOverlayInterface::createHoverOverlay(entityItemID, PointerEvent());
}

void HoverOverlayInterface::destroyHoverOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    qCDebug(hover_overlay) << "Destroying Hover Overlay on top of entity with ID: " << entityItemID;
    setCurrentHoveredEntity(QUuid());
}

void HoverOverlayInterface::destroyHoverOverlay(const EntityItemID& entityItemID) {
    HoverOverlayInterface::destroyHoverOverlay(entityItemID, PointerEvent());
}
