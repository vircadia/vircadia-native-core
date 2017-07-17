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

}

void HoverOverlayInterface::createHoverOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_verboseLogging) {
        qDebug() << "Creating Hover Overlay on top of entity with ID: " << entityItemID;
    }
    setCurrentHoveredEntity(entityItemID);
}

void HoverOverlayInterface::destroyHoverOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_verboseLogging) {
        qDebug() << "Destroying Hover Overlay on top of entity with ID: " << entityItemID;
    }
    setCurrentHoveredEntity(QUuid());
}
