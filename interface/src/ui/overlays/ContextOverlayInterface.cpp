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
#include <NetworkingConstants.h>

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
    _selectionScriptingInterface = DependencyManager::get<SelectionScriptingInterface>();

    _selectionToSceneHandler.initialize("contextOverlayHighlightList");

    _entityPropertyFlags += PROP_POSITION;
    _entityPropertyFlags += PROP_ROTATION;
    _entityPropertyFlags += PROP_MARKETPLACE_ID;
    _entityPropertyFlags += PROP_DIMENSIONS;
    _entityPropertyFlags += PROP_REGISTRATION_POINT;

    // initially, set _enabled to match the switch.  Later we enable/disable via the getter/setters
    // if we are in edit or pal (for instance).  Note this is temporary, as we expect to enable this all
    // the time after getting edge highlighting, etc...
    _enabled = _settingSwitch.get();

    auto entityTreeRenderer = DependencyManager::get<EntityTreeRenderer>().data();
    connect(entityTreeRenderer, SIGNAL(mousePressOnEntity(const EntityItemID&, const PointerEvent&)), this, SLOT(createOrDestroyContextOverlay(const EntityItemID&, const PointerEvent&)));
    connect(entityTreeRenderer, SIGNAL(hoverEnterEntity(const EntityItemID&, const PointerEvent&)), this, SLOT(contextOverlays_hoverEnterEntity(const EntityItemID&, const PointerEvent&)));
    connect(entityTreeRenderer, SIGNAL(hoverLeaveEntity(const EntityItemID&, const PointerEvent&)), this, SLOT(contextOverlays_hoverLeaveEntity(const EntityItemID&, const PointerEvent&)));
    connect(_tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"), &TabletProxy::tabletShownChanged, this, [&]() {
        if (_contextOverlayJustClicked && _hmdScriptingInterface->isMounted()) {
            QUuid tabletFrameID = _hmdScriptingInterface->getCurrentTabletFrameID();
            auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
            glm::quat cameraOrientation = qApp->getCamera().getOrientation();
            QVariantMap props;
            props.insert("position", vec3toVariant(myAvatar->getEyePosition() + glm::quat(glm::radians(glm::vec3(0.0f, CONTEXT_OVERLAY_TABLET_OFFSET, 0.0f))) * (CONTEXT_OVERLAY_TABLET_DISTANCE * (cameraOrientation * Vectors::FRONT))));
            props.insert("orientation", quatToVariant(cameraOrientation * glm::quat(glm::radians(glm::vec3(0.0f, CONTEXT_OVERLAY_TABLET_ORIENTATION, 0.0f)))));
            qApp->getOverlays().editOverlay(tabletFrameID, props);
            _contextOverlayJustClicked = false;
        }
    });
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>().data();
    connect(entityScriptingInterface, &EntityScriptingInterface::deletingEntity, this, &ContextOverlayInterface::deletingEntity);

    connect(_selectionScriptingInterface.data(), &SelectionScriptingInterface::selectedItemsListChanged, &_selectionToSceneHandler, &SelectionToSceneHandler::selectedItemsListChanged);
}

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

void ContextOverlayInterface::setEnabled(bool enabled) {
    // only enable/disable if the setting in 'on'.   If it is 'off',
    // make sure _enabled is always false.
    if (_settingSwitch.get()) {
        _enabled = enabled;
    } else {
        _enabled = false;
    }
}

bool ContextOverlayInterface::createOrDestroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_enabled && event.getButton() == PointerEvent::SecondaryButton) {
        if (contextOverlayFilterPassed(entityItemID)) {
            qCDebug(context_overlay) << "Creating Context Overlay on top of entity with ID: " << entityItemID;

            // Add all necessary variables to the stack
            EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);
            glm::vec3 cameraPosition = qApp->getCamera().getPosition();
            float distanceFromCameraToEntity = glm::distance(entityProperties.getPosition(), cameraPosition);
            glm::vec3 entityDimensions = entityProperties.getDimensions();
            glm::vec3 entityPosition = entityProperties.getPosition();
            glm::vec3 contextOverlayPosition = entityProperties.getPosition();
            glm::vec2 contextOverlayDimensions;

            // Update the position of the overlay if the registration point of the entity
            // isn't default
            if (entityProperties.getRegistrationPoint() != glm::vec3(0.5f)) {
                glm::vec3 adjustPos = entityProperties.getRegistrationPoint() - glm::vec3(0.5f);
                entityPosition = entityPosition - (entityProperties.getRotation() * (adjustPos * entityProperties.getDimensions()));
            }

            enableEntityHighlight(entityItemID);

            AABox boundingBox = AABox(entityPosition - (entityDimensions / 2.0f), entityDimensions * 2.0f);

            // Update the cached Entity Marketplace ID
            _entityMarketplaceID = entityProperties.getMarketplaceID();


            if (!_currentEntityWithContextOverlay.isNull() && _currentEntityWithContextOverlay != entityItemID) {
                disableEntityHighlight(_currentEntityWithContextOverlay);
            }

            // Update the cached "Current Entity with Context Overlay" variable
            setCurrentEntityWithContextOverlay(entityItemID);

            // Here, we determine the position and dimensions of the Context Overlay.
            if (boundingBox.contains(cameraPosition)) {
                // If the camera is inside the box...
                // ...position the Context Overlay 1 meter in front of the camera.
                contextOverlayPosition = cameraPosition + CONTEXT_OVERLAY_INSIDE_DISTANCE * (qApp->getCamera().getOrientation() * Vectors::FRONT);
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_CLOSE_SIZE, CONTEXT_OVERLAY_CLOSE_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            } else if (distanceFromCameraToEntity < CONTEXT_OVERLAY_CLOSE_DISTANCE) {
                // Else if the entity is too close to the camera...
                // ...rotate the Context Overlay to the right of the entity.
                // This makes it easy to inspect things you're holding.
                float offsetAngle = -CONTEXT_OVERLAY_CLOSE_OFFSET_ANGLE;
                if (event.getID() == LEFT_HAND_HW_ID) {
                    offsetAngle *= -1;
                }
                contextOverlayPosition = (glm::quat(glm::radians(glm::vec3(0.0f, offsetAngle, 0.0f))) * (entityPosition - cameraPosition)) + cameraPosition;
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_CLOSE_SIZE, CONTEXT_OVERLAY_CLOSE_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            } else {
                // Else, place the Context Overlay some offset away from the entity's bounding
                // box in the direction of the camera.
                glm::vec3 direction = glm::normalize(entityPosition - cameraPosition);
                float distance;
                BoxFace face;
                glm::vec3 normal;
                boundingBox.findRayIntersection(cameraPosition, direction, distance, face, normal);
                contextOverlayPosition = (cameraPosition + direction * distance) - direction * CONTEXT_OVERLAY_FAR_OFFSET;
                contextOverlayDimensions = glm::vec2(CONTEXT_OVERLAY_FAR_SIZE, CONTEXT_OVERLAY_FAR_SIZE) * glm::distance(contextOverlayPosition, cameraPosition);
            }

            // Finally, setup and draw the Context Overlay
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
            _contextOverlay->setPosition(contextOverlayPosition);
            _contextOverlay->setDimensions(contextOverlayDimensions);
            _contextOverlay->setRotation(entityProperties.getRotation());
            _contextOverlay->setVisible(true);

            return true;
        }
    } else {
        if (!_currentEntityWithContextOverlay.isNull()) {
            return destroyContextOverlay(_currentEntityWithContextOverlay, event);
        }
        return false;
    }
    return false;
}

bool ContextOverlayInterface::contextOverlayFilterPassed(const EntityItemID& entityItemID) {
    EntityItemProperties entityProperties = _entityScriptingInterface->getEntityProperties(entityItemID, _entityPropertyFlags);
    return (entityProperties.getMarketplaceID().length() != 0);
}

bool ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event) {
    if (_contextOverlayID != UNKNOWN_OVERLAY_ID) {
        qCDebug(context_overlay) << "Destroying Context Overlay on top of entity with ID: " << entityItemID;
        disableEntityHighlight(entityItemID);
        setCurrentEntityWithContextOverlay(QUuid());
        _entityMarketplaceID.clear();
        // Destroy the Context Overlay
        qApp->getOverlays().deleteOverlay(_contextOverlayID);
        _contextOverlay = NULL;
        _contextOverlayID = UNKNOWN_OVERLAY_ID;
        return true;
    }
    return false;
}

bool ContextOverlayInterface::destroyContextOverlay(const EntityItemID& entityItemID) {
    return ContextOverlayInterface::destroyContextOverlay(entityItemID, PointerEvent());
}

void ContextOverlayInterface::contextOverlays_mousePressOnOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (overlayID == _contextOverlayID  && event.getButton() == PointerEvent::PrimaryButton) {
        qCDebug(context_overlay) << "Clicked Context Overlay. Entity ID:" << _currentEntityWithContextOverlay << "Overlay ID:" << overlayID;
        openMarketplace();
        destroyContextOverlay(_currentEntityWithContextOverlay, PointerEvent());
        _contextOverlayJustClicked = true;
    }
}

void ContextOverlayInterface::contextOverlays_hoverEnterOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (_contextOverlayID != UNKNOWN_OVERLAY_ID && _contextOverlay) {
        qCDebug(context_overlay) << "Started hovering over Context Overlay. Overlay ID:" << overlayID;
        _contextOverlay->setColor(CONTEXT_OVERLAY_COLOR);
        _contextOverlay->setColorPulse(0.0f); // pulse off
        _contextOverlay->setPulsePeriod(0.0f); // pulse off
        _contextOverlay->setAlpha(CONTEXT_OVERLAY_HOVERED_ALPHA);
    }
}

void ContextOverlayInterface::contextOverlays_hoverLeaveOverlay(const OverlayID& overlayID, const PointerEvent& event) {
    if (_contextOverlayID != UNKNOWN_OVERLAY_ID && _contextOverlay) {
        qCDebug(context_overlay) << "Stopped hovering over Context Overlay. Overlay ID:" << overlayID;
        _contextOverlay->setColor(CONTEXT_OVERLAY_COLOR);
        _contextOverlay->setColorPulse(CONTEXT_OVERLAY_UNHOVERED_COLORPULSE);
        _contextOverlay->setPulsePeriod(CONTEXT_OVERLAY_UNHOVERED_PULSEPERIOD);
        _contextOverlay->setAlpha(CONTEXT_OVERLAY_UNHOVERED_ALPHA);
    }
}

void ContextOverlayInterface::contextOverlays_hoverEnterEntity(const EntityItemID& entityID, const PointerEvent& event) {
    if (contextOverlayFilterPassed(entityID)) {
        enableEntityHighlight(entityID);
    }
}

void ContextOverlayInterface::contextOverlays_hoverLeaveEntity(const EntityItemID& entityID, const PointerEvent& event) {
    if (_currentEntityWithContextOverlay != entityID) {
        disableEntityHighlight(entityID);
    }
}

static const QString MARKETPLACE_BASE_URL = NetworkingConstants::METAVERSE_SERVER_URL.toString() + "/marketplace/items/";

void ContextOverlayInterface::openMarketplace() {
    // lets open the tablet and go to the current item in
    // the marketplace (if the current entity has a
    // marketplaceID)
    if (!_currentEntityWithContextOverlay.isNull() && _entityMarketplaceID.length() > 0) {
        auto tablet = dynamic_cast<TabletProxy*>(_tabletScriptingInterface->getTablet("com.highfidelity.interface.tablet.system"));
        // construct the url to the marketplace item
        QString url = MARKETPLACE_BASE_URL + _entityMarketplaceID;
        QString MARKETPLACES_INJECT_SCRIPT_PATH = "file:///" + qApp->applicationDirPath() + "/scripts/system/html/js/marketplacesInject.js";
        tablet->gotoWebScreen(url, MARKETPLACES_INJECT_SCRIPT_PATH);
        _hmdScriptingInterface->openTablet();
        _isInMarketplaceInspectionMode = true;
    }
}

void ContextOverlayInterface::enableEntityHighlight(const EntityItemID& entityItemID) {
    _selectionScriptingInterface->addToSelectedItemsList("contextOverlayHighlightList", "entity", entityItemID);
}

void ContextOverlayInterface::disableEntityHighlight(const EntityItemID& entityItemID) {
    _selectionScriptingInterface->removeFromSelectedItemsList("contextOverlayHighlightList", "entity", entityItemID);
}

void ContextOverlayInterface::deletingEntity(const EntityItemID& entityID) {
    if (_currentEntityWithContextOverlay == entityID) {
        destroyContextOverlay(_currentEntityWithContextOverlay, PointerEvent());
    }
}
