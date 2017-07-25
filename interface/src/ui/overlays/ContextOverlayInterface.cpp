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

static const float CONTEXT_OVERLAY_TABLET_OFFSET = 30.0f; // Degrees
static const float CONTEXT_OVERLAY_TABLET_ORIENTATION = 210.0f; // Degrees
static const float CONTEXT_OVERLAY_TABLET_DISTANCE = 0.65F; // Meters
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
    connect(_tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"), &TabletProxy::tabletShownChanged, this, [&]() {
        if (_contextOverlayJustClicked && _hmdScriptingInterface->isMounted()) {
            QUuid tabletFrameID = _hmdScriptingInterface->getCurrentTabletFrameID();
            QVariantMap props;
            auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
            glm::quat cameraOrientation = qApp->getCamera().getOrientation();
            props.insert("position", vec3toVariant(myAvatar->getEyePosition() + glm::quat(glm::radians(glm::vec3(0.0f, CONTEXT_OVERLAY_TABLET_OFFSET, 0.0f))) * (CONTEXT_OVERLAY_TABLET_DISTANCE * (cameraOrientation * Vectors::FRONT))));
            props.insert("orientation", quatToVariant(cameraOrientation * glm::quat(glm::radians(glm::vec3(0.0f, CONTEXT_OVERLAY_TABLET_ORIENTATION, 0.0f)))));
            qApp->getOverlays().editOverlay(tabletFrameID, props);
            _contextOverlayJustClicked = false;
        }
    });
}

static const xColor BB_OVERLAY_COLOR = {255, 255, 0};
static const uint32_t LEFT_HAND_HW_ID = 1;
static const xColor CONTEXT_OVERLAY_COLOR = { 255, 255, 255 };
static const float CONTEXT_OVERLAY_INSIDE_DISTANCE = 1.0f; // in meters
static const float CONTEXT_OVERLAY_CLOSE_DISTANCE = 1.5f; // in meters
static const float CONTEXT_OVERLAY_CLOSE_SIZE = 0.12f; // in meters, same x and y dims
static const float CONTEXT_OVERLAY_FAR_SIZE = 0.08f; // in meters, same x and y dims
static const float CONTEXT_OVERLAY_CLOSE_OFFSET_ANGLE = 20.0f;
static const float CONTEXT_OVERLAY_UNHOVERED_ALPHA = 0.85f;
static const float CONTEXT_OVERLAY_HOVERED_ALPHA = 1.0f;
static const float CONTEXT_OVERLAY_UNHOVERED_PULSEMIN = 0.6f;
static const float CONTEXT_OVERLAY_UNHOVERED_PULSEMAX = 1.0f;
static const float CONTEXT_OVERLAY_UNHOVERED_PULSEPERIOD = 1.0f;
static const float CONTEXT_OVERLAY_UNHOVERED_COLORPULSE = 1.0f;
static const float CONTEXT_OVERLAY_FAR_OFFSET = 0.1f;

bool ContextOverlayInterface::createOrDestroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_enabled && event.getButton() == PointerEvent::SecondaryButton) {
        EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);
        glm::vec3 bbPosition = entityProperties.getPosition();
        glm::vec3 dimensions = entityProperties.getDimensions();
        if (entityProperties.getRegistrationPoint() != glm::vec3(0.5f)) {
            glm::vec3 adjustPos = entityProperties.getRegistrationPoint() - glm::vec3(0.5f);
            bbPosition = bbPosition - (entityProperties.getRotation() * (adjustPos * dimensions));
        }
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
                _bbOverlayID = qApp->getOverlays().addOverlay(_bbOverlay);
            }
            _bbOverlay->setParentID(entityItemID);
            _bbOverlay->setDimensions(dimensions);
            _bbOverlay->setRotation(entityProperties.getRotation());
            _bbOverlay->setPosition(bbPosition);
            _bbOverlay->setVisible(true);

            if (_contextOverlayID == UNKNOWN_OVERLAY_ID || !qApp->getOverlays().isAddedOverlay(_contextOverlayID)) {
                _contextOverlay = std::make_shared<Image3DOverlay>();
                _contextOverlay->setAlpha(CONTEXT_OVERLAY_UNHOVERED_ALPHA);
                _contextOverlay->setPulseMin(CONTEXT_OVERLAY_UNHOVERED_PULSEMIN);
                _contextOverlay->setPulseMax(CONTEXT_OVERLAY_UNHOVERED_PULSEMAX);
                _contextOverlay->setColorPulse(CONTEXT_OVERLAY_UNHOVERED_COLORPULSE);
                _contextOverlay->setIgnoreRayIntersection(false);
                _contextOverlay->setDrawInFront(true);
                _contextOverlay->setURL(PathUtils::resourcesPath() + "images/inspect-icon.png");
                _contextOverlay->setIsFacingAvatar(true);
                _contextOverlayID = qApp->getOverlays().addOverlay(_contextOverlay);
            }
            glm::vec3 cameraPosition = qApp->getCamera().getPosition();
            float distanceToEntity = glm::distance(bbPosition, cameraPosition);
            glm::vec3 contextOverlayPosition;
            glm::vec2 contextOverlayDimensions;
            if (AABox(bbPosition - (dimensions / 2.0f), dimensions * 2.0f).contains(cameraPosition)) {
                // If the camera is inside the box, position the context overlay 1 meter in front of the camera.
                contextOverlayPosition = cameraPosition + CONTEXT_OVERLAY_INSIDE_DISTANCE * (qApp->getCamera().getOrientation() * Vectors::FRONT);
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_CLOSE_SIZE, CONTEXT_OVERLAY_CLOSE_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            } else if (distanceToEntity < CONTEXT_OVERLAY_CLOSE_DISTANCE) {
                // If the entity is too close to the camera, rotate the context overlay to the right of the entity.
                // This makes it easy to inspect things you're holding.
                float offsetAngle = -CONTEXT_OVERLAY_CLOSE_OFFSET_ANGLE;
                if (event.getID() == LEFT_HAND_HW_ID) {
                    offsetAngle *= -1;
                }
                contextOverlayPosition = (glm::quat(glm::radians(glm::vec3(0.0f, offsetAngle, 0.0f))) * (entityProperties.getPosition() - cameraPosition)) + cameraPosition;
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_CLOSE_SIZE, CONTEXT_OVERLAY_CLOSE_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            } else {
                auto direction = glm::normalize(bbPosition - cameraPosition);
                PickRay pickRay(cameraPosition, direction);
                _bbOverlay->setIgnoreRayIntersection(false);
                auto result = qApp->getOverlays().findRayIntersection(pickRay);
                _bbOverlay->setIgnoreRayIntersection(true);
                contextOverlayPosition = result.intersection - direction * CONTEXT_OVERLAY_FAR_OFFSET;
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_FAR_SIZE, CONTEXT_OVERLAY_FAR_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            }
            _contextOverlay->setPosition(contextOverlayPosition);
            _contextOverlay->setDimensions(contextOverlayDimensions);
            _contextOverlay->setRotation(entityProperties.getRotation());
            _contextOverlay->setVisible(true);
            return true;
        }
    } else {
        return destroyContextOverlay(entityItemID, event);
    }
    return false;
}

bool ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_contextOverlayID != UNKNOWN_OVERLAY_ID) {
        qCDebug(context_overlay) << "Destroying Context Overlay on top of entity with ID: " << entityItemID;
        setCurrentEntityWithContextOverlay(QUuid());
        qApp->getOverlays().deleteOverlay(_contextOverlayID);
        qApp->getOverlays().deleteOverlay(_bbOverlayID);
        _contextOverlay = NULL;
        _bbOverlay = NULL;
        _contextOverlayID = UNKNOWN_OVERLAY_ID;
        _bbOverlayID = UNKNOWN_OVERLAY_ID;
        _entityMarketplaceID.clear();
        return true;
    }
    return false;
}

bool ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID) {
    return ContextOverlayInterface::destroyContextOverlay(entityItemID, PointerEvent());
}

void ContextOverlayInterface::clickContextOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (overlayID == _contextOverlayID  && event.getButton() == PointerEvent::PrimaryButton) {
        qCDebug(context_overlay) << "Clicked Context Overlay. Entity ID:" << _currentEntityWithContextOverlay << "Overlay ID:" << overlayID;
        openMarketplace();
        destroyContextOverlay(_currentEntityWithContextOverlay, PointerEvent());
        _contextOverlayJustClicked = true;
    }
}

void ContextOverlayInterface::hoverEnterContextOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (_contextOverlayID != UNKNOWN_OVERLAY_ID && _contextOverlay) {
        qCDebug(context_overlay) << "Started hovering over Context Overlay. Overlay ID:" << overlayID;
        _contextOverlay->setColor(CONTEXT_OVERLAY_COLOR);
        _contextOverlay->setColorPulse(0.0f); // pulse off
        _contextOverlay->setPulsePeriod(0.0f); // pulse off
        _contextOverlay->setAlpha(CONTEXT_OVERLAY_HOVERED_ALPHA);
    }
}

void ContextOverlayInterface::hoverLeaveContextOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (_contextOverlayID != UNKNOWN_OVERLAY_ID && _contextOverlay) {
        qCDebug(context_overlay) << "Stopped hovering over Context Overlay. Overlay ID:" << overlayID;
        _contextOverlay->setColor(CONTEXT_OVERLAY_COLOR);
        _contextOverlay->setColorPulse(CONTEXT_OVERLAY_UNHOVERED_COLORPULSE);
        _contextOverlay->setPulsePeriod(CONTEXT_OVERLAY_UNHOVERED_PULSEPERIOD);
        _contextOverlay->setAlpha(CONTEXT_OVERLAY_UNHOVERED_ALPHA);
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
