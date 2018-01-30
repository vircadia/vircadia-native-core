//
//  ContextOverlayInterface.h
//  interface/src/ui/overlays
//
//  Created by Zach Fox on 2017-07-14.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_ContextOverlayInterface_h
#define hifi_ContextOverlayInterface_h

#include <QtCore/QObject>
#include <QUuid>

#include <DependencyManager.h>
#include <PointerEvent.h>
#include <ui/TabletScriptingInterface.h>
#include "avatar/AvatarManager.h"

#include "EntityScriptingInterface.h"
#include "ui/overlays/Image3DOverlay.h"
#include "ui/overlays/Overlays.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/SelectionScriptingInterface.h"
#include "scripting/WalletScriptingInterface.h"

#include "EntityTree.h"
#include "ContextOverlayLogging.h"

/**jsdoc
* @namespace ContextOverlay
*/
class ContextOverlayInterface : public QObject, public Dependency {
    Q_OBJECT

    Q_PROPERTY(QUuid entityWithContextOverlay READ getCurrentEntityWithContextOverlay WRITE setCurrentEntityWithContextOverlay)
    Q_PROPERTY(bool enabled READ getEnabled WRITE setEnabled)
    Q_PROPERTY(bool isInMarketplaceInspectionMode READ getIsInMarketplaceInspectionMode WRITE setIsInMarketplaceInspectionMode)

    QSharedPointer<EntityScriptingInterface> _entityScriptingInterface;
    EntityPropertyFlags _entityPropertyFlags;
    QSharedPointer<HMDScriptingInterface> _hmdScriptingInterface;
    QSharedPointer<TabletScriptingInterface> _tabletScriptingInterface;
    QSharedPointer<SelectionScriptingInterface> _selectionScriptingInterface;
    OverlayID _contextOverlayID { UNKNOWN_OVERLAY_ID };
    std::shared_ptr<Image3DOverlay> _contextOverlay { nullptr };
public:
    ContextOverlayInterface();
    Q_INVOKABLE QUuid getCurrentEntityWithContextOverlay() { return _currentEntityWithContextOverlay; }
    void setCurrentEntityWithContextOverlay(const QUuid& entityID) { _currentEntityWithContextOverlay = entityID; }
    void setLastInspectedEntity(const QUuid& entityID) { _challengeOwnershipTimeoutTimer.stop(); _lastInspectedEntity = entityID; }
    void setEnabled(bool enabled);
    bool getEnabled() { return _enabled; }
    bool getIsInMarketplaceInspectionMode() { return _isInMarketplaceInspectionMode; }
    void setIsInMarketplaceInspectionMode(bool mode) { _isInMarketplaceInspectionMode = mode; }
    void requestOwnershipVerification(const QUuid& entityID);
    EntityPropertyFlags getEntityPropertyFlags() { return _entityPropertyFlags; }

signals:
    void contextOverlayClicked(const QUuid& currentEntityWithContextOverlay);

public slots:
    bool createOrDestroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);
    bool destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);
    bool destroyContextOverlay(const EntityItemID& entityItemID);
    void contextOverlays_mousePressOnOverlay(const OverlayID& overlayID, const PointerEvent& event);
    void contextOverlays_hoverEnterOverlay(const OverlayID& overlayID, const PointerEvent& event);
    void contextOverlays_hoverLeaveOverlay(const OverlayID& overlayID, const PointerEvent& event);
    void contextOverlays_hoverEnterEntity(const EntityItemID& entityID, const PointerEvent& event);
    void contextOverlays_hoverLeaveEntity(const EntityItemID& entityID, const PointerEvent& event);
    bool contextOverlayFilterPassed(const EntityItemID& entityItemID);

private slots:
    void handleChallengeOwnershipReplyPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);

private:

    enum {
        MAX_SELECTION_COUNT = 16
    };
    bool _verboseLogging{ true };
    bool _enabled { true };
    EntityItemID _currentEntityWithContextOverlay{};
    EntityItemID _lastInspectedEntity{};
    QString _entityMarketplaceID;
    bool _contextOverlayJustClicked { false };

    bool _isInMarketplaceInspectionMode { false };

    void openInspectionCertificate();
    void openMarketplace();
    void enableEntityHighlight(const EntityItemID& entityItemID);
    void disableEntityHighlight(const EntityItemID& entityItemID);

    void deletingEntity(const EntityItemID& entityItemID);

    Q_INVOKABLE void startChallengeOwnershipTimer();
    QTimer _challengeOwnershipTimeoutTimer;
};

#endif // hifi_ContextOverlayInterface_h
