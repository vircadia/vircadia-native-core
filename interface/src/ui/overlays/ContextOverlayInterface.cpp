//
//  ContextOverlayInterface.cpp
//  interface/src/ui/overlays
//
//  Created by Zach Fox on 2017-07-14.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ContextOverlayInterface.h"
#include "Application.h"

#include <EntityTreeRenderer.h>

ContextOverlayInterface::ContextOverlayInterface() {
    // "context_overlay" debug log category disabled by default.
    // Create your own "qtlogging.ini" file and set your "QT_LOGGING_CONF" environment variable
    // if you'd like to enable/disable certain categories.
    // More details: http://doc.qt.io/qt-5/qloggingcategory.html#configuring-categories
    QLoggingCategory::setFilterRules(QStringLiteral("hifi.context_overlay.debug=false"));

    _entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    _hmdScriptingInterface = DependencyManager::get<HMDScriptingInterface>();
    _tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();

    _entityPropertyFlags += PROP_POSITION;
    _entityPropertyFlags += PROP_ROTATION;
    _entityPropertyFlags += PROP_MARKETPLACE_ID;
    _entityPropertyFlags += PROP_DIMENSIONS;
    _entityPropertyFlags += PROP_REGISTRATION_POINT;

    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>().data();
    connect(entityTreeRenderer, SIGNAL(mousePressOnEntity(const EntityItemID&, const PointerEvent&)), this, SLOT(createOrDestroyContextOverlay(const EntityItemID&, const PointerEvent&)));
}

static const xColor BB_OVERLAY_COLOR = {255, 255, 0};

void ContextOverlayInterface::createOrDestroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_enabled && event.getButton() == PointerEvent::SecondaryButton) {

        EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);
        glm::vec3 position = entityProperties.getPosition();
        if (entityProperties.getMarketplaceID().length() != 0) {
            qCDebug(context_overlay) << "Creating Context Overlay on top of entity with ID: " << entityItemID;
            _entityMarketplaceID = entityProperties.getMarketplaceID();
            setCurrentEntityWithContextOverlay(entityItemID);

            if (_bbOverlayID == UNKNOWN_OVERLAY_ID || !qApp->getOverlays().isAddedOverlay(_bbOverlayID)) {
                _bbOverlay = std::make_shared<Cube3DOverlay>();
                _bbOverlay->setIsSolid(false);
                _bbOverlay->setColor(BB_OVERLAY_COLOR);
                _bbOverlay->setDrawInFront(true);
                _bbOverlay->setIgnoreRayIntersection(true);
                _bbOverlay->setParentID(entityItemID);
                _bbOverlayID = qApp->getOverlays().addOverlay(_bbOverlay);
            }
            _bbOverlay->setDimensions(entityProperties.getDimensions());
            _bbOverlay->setRotation(entityProperties.getRotation());
            _bbOverlay->setPosition(position);
            _bbOverlay->setVisible(true);

            if (_contextOverlayID == UNKNOWN_OVERLAY_ID || !qApp->getOverlays().isAddedOverlay(_contextOverlayID)) {
                _contextOverlay = std::make_shared<Image3DOverlay>();
                _contextOverlay->setAlpha(1.0f);
                _contextOverlay->setPulseMin(0.75f);
                _contextOverlay->setPulseMax(1.0f);
                _contextOverlay->setColorPulse(1.0f);
                _contextOverlay->setIgnoreRayIntersection(false);
                _contextOverlay->setDrawInFront(true);
                _contextOverlay->setURL("http://i.imgur.com/gksZygp.png");
                _contextOverlay->setIsFacingAvatar(true);
                _contextOverlay->setParentID(entityItemID);
                _contextOverlayID = qApp->getOverlays().addOverlay(_contextOverlay);
            }

            _contextOverlay->setDimensions(glm::vec2(0.05f, 0.05f) * glm::distance(entityProperties.getPosition(), qApp->getCamera().getPosition()));
            _contextOverlay->setPosition(position);
            _contextOverlay->setRotation(entityProperties.getRotation());
            _contextOverlay->setVisible(true);
        }
    } else if (_currentEntityWithContextOverlay == entityItemID) {
        destroyContextOverlay(entityItemID, event);
    }
}

void ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    qCDebug(context_overlay) << "Destroying Context Overlay on top of entity with ID: " << entityItemID;
    setCurrentEntityWithContextOverlay(QUuid());

    qApp->getOverlays().deleteOverlay(_contextOverlayID);
    qApp->getOverlays().deleteOverlay(_bbOverlayID);
    _contextOverlay = NULL;
    _bbOverlay = NULL;
    _contextOverlayID = UNKNOWN_OVERLAY_ID;
    _bbOverlayID = UNKNOWN_OVERLAY_ID;
    _entityMarketplaceID.clear();
}

void ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID) {
    ContextOverlayInterface::destroyContextOverlay(entityItemID, PointerEvent());
}

void ContextOverlayInterface::clickContextOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (overlayID == _contextOverlayID  && event.getButton() == PointerEvent::PrimaryButton) {
        qCDebug(context_overlay) << "Clicked Context Overlay. Entity ID:" << _currentEntityWithContextOverlay << "Overlay ID:" << overlayID;
        openMarketplace();
        destroyContextOverlay(_currentEntityWithContextOverlay, PointerEvent());
    }
}
static const QString MARKETPLACE_BASE_URL = "http://metaverse.highfidelity.com/marketplace/items/";

void ContextOverlayInterface::openMarketplace() {
    // lets open the tablet and go to the current item in
    // the marketplace (if the current entity has a
    // marketplaceID)
    if (!_currentEntityWithContextOverlay.isNull() && _entityMarketplaceID.length() > 0) {
        auto tablet = dynamic_cast<TabletProxy*>(_tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));
        // construct the url to the marketplace item
        QString url = MARKETPLACE_BASE_URL + _entityMarketplaceID;
        tablet->gotoWebScreen(url);
        _hmdScriptingInterface->openTablet();
    }
}
