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
#include <QtCore/QSharedPointer>

#include <DependencyManager.h>
#include <PointerEvent.h>
#include <ui/TabletScriptingInterface.h>
#include "avatar/AvatarManager.h"

#include "EntityScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/SelectionScriptingInterface.h"
#include "scripting/WalletScriptingInterface.h"

#include "EntityTree.h"
#include "ContextOverlayLogging.h"

/*@jsdoc
 * The <code>ContextOverlay</code> API manages the "i" proof-of-provenance context overlay that appears on Marketplace items 
 * when a user right-clicks them.
 *
 * @namespace ContextOverlay
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {boolean} enabled - <code>true</code> if the context overlay is enabled to be displayed, <code>false</code> if it 
 *     is disabled and will never be displayed.
 * @property {Uuid} entityWithContextOverlay - The ID of the entity that the context overlay is currently displayed for,
 *     <code>null</code> if the context overlay is not currently displayed.
 * @property {boolean} isInMarketplaceInspectionMode - <em>Currently not used.</em>
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
    QUuid _contextOverlayID { UNKNOWN_ENTITY_ID };
public:
    ContextOverlayInterface();

    /*@jsdoc
     * Gets the ID of the entity that the context overlay is currently displayed for.
     * @function ContextOverlay.getCurrentEntityWithContextOverlay
     * @returns {Uuid} - The ID of the entity that the context overlay is currently displayed for, <code>null</code> if the 
     *     context overlay is not currently displayed.
     */
    Q_INVOKABLE QUuid getCurrentEntityWithContextOverlay() { return _currentEntityWithContextOverlay; }

    void setCurrentEntityWithContextOverlay(const QUuid& entityID) { _currentEntityWithContextOverlay = entityID; }
    void setLastInspectedEntity(const QUuid& entityID) { _challengeOwnershipTimeoutTimer.stop(); }
    void setEnabled(bool enabled);
    bool getEnabled() { return _enabled; }
    bool getIsInMarketplaceInspectionMode() { return _isInMarketplaceInspectionMode; }
    void setIsInMarketplaceInspectionMode(bool mode) { _isInMarketplaceInspectionMode = mode; }

    /*@jsdoc
     * Initiates a check on an avatar entity belongs to the user wearing it. The result is returned via 
     * {@link WalletScriptingInterface.ownershipVerificationSuccess} or 
     * {@link WalletScriptingInterface.ownershipVerificationFailed}.
     * <p><strong>Warning:</strong> Neither of these signals are triggered if the entity is not an avatar entity or is not 
     * certified.</p>
     * @function ContextOverlay.requestOwnershipVerification
     * @param {Uuid} entityID - The ID of the entity to check.
     */
    Q_INVOKABLE void requestOwnershipVerification(const QUuid& entityID);

    EntityPropertyFlags getEntityPropertyFlags() { return _entityPropertyFlags; }

signals:
    /*@jsdoc
     * Triggered when the user clicks on the context overlay.
     * @function ContextOverlay.contextOverlayClicked
     * @param {Uuid} id - The ID of the entity that the context overlay is for.
     * @returns {Signal}
     * @example <caption>Report when a context overlay is clicked.</caption>
     * ContextOverlay.contextOverlayClicked.connect(function (id) {
     *     print("Context overlay clicked for:", id);
     * });
     */
    void contextOverlayClicked(const QUuid& currentEntityWithContextOverlay);

public slots:

    /*@jsdoc
     * @function ContextOverlay.clickDownOnEntity
     * @param {Uuid} id - Entity ID.
     * @param {PointerEvent} event - Pointer event.
     * @deprecated This method is deprecated and will be removed.
     */
    // FIXME: Method shouldn't be in the API.
    void clickDownOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    /*@jsdoc
     * @function ContextOverlay.mouseReleaseOnEntity
     * @param {Uuid} id - Entity ID.
     * @param {PointerEvent} event - Pointer event.
     * @deprecated This method is deprecated and will be removed.
     */
    // FIXME: Method shouldn't be in the API.
    void mouseReleaseOnEntity(const EntityItemID& entityItemID, const PointerEvent& event);

    /*@jsdoc
     * Displays or deletes the context overlay as appropriate for the target entity and a pointer event: the context overlay 
     * must be enabled and the pointer event must be a right-click; if so, then any current context overlay is deleted, and if 
     * the target entity should have a context overlay then it is displayed.
     * @function ContextOverlay.createOrDestroyContextOverlay
     * @param {Uuid} entityID - The target entity.
     * @param {PointerEvent} pointerEvent - The pointer event.
     * @returns {boolean} - <code>true</code> if the context overlay was deleted or displayed on the specified entity, 
     *     <code>false</code> if no action was taken.
     */
    bool createOrDestroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);

    /*@jsdoc
     * Deletes the context overlay and removes the entity highlight, if shown.
     * @function ContextOverlay.destroyContextOverlay
     * @param {Uuid} entityID - The ID of the entity.
     * @param {PointerEvent} [event] - <em>Not used.</em>
     * @returns {boolean} - <code>true</code> if the context overlay was deleted, <code>false</code> if it wasn't (e.g., it 
     *     wasn't displayed).
     */
    bool destroyContextOverlay(const EntityItemID& entityItemID, const PointerEvent& event);
    bool destroyContextOverlay(const EntityItemID& entityItemID);

    /*@jsdoc
     * @function ContextOverlay.contextOverlays_hoverEnterOverlay
     * @param {Uuid} id - Overlay ID.
     * @param {PointerEvent} event - Pointer event.
     * @deprecated This method is deprecated and will be removed.
     */
    // FIXME: Method shouldn't be in the API.
    void contextOverlays_hoverEnterOverlay(const QUuid& id, const PointerEvent& event);

    /*@jsdoc
     * @function ContextOverlay.contextOverlays_hoverLeaveOverlay
     * @param {Uuid} id - Overlay ID.
     * @param {PointerEvent} event - Pointer event.
     * @deprecated This method is deprecated and will be removed.
     */
    // FIXME: Method shouldn't be in the API.
    void contextOverlays_hoverLeaveOverlay(const QUuid& id, const PointerEvent& event);

    /*@jsdoc
     * @function ContextOverlay.contextOverlays_hoverEnterEntity
     * @param {Uuid} id - Entity ID.
     * @param {PointerEvent} event - Pointer event.
     * @deprecated This method is deprecated and will be removed.
     */
    // FIXME: Method shouldn't be in the API.
    void contextOverlays_hoverEnterEntity(const EntityItemID& entityID, const PointerEvent& event);

    /*@jsdoc
     * @function ContextOverlay.contextOverlays_hoverLeaveEntity
     * @param {Uuid} id - Entity ID.
     * @param {PointerEvent} event - Pointer event.
     * @deprecated This method is deprecated and will be removed.
     */
    // FIXME: Method shouldn't be in the API.
    void contextOverlays_hoverLeaveEntity(const EntityItemID& entityID, const PointerEvent& event);

    /*@jsdoc
     * Checks with a context overlay should be displayed for an entity &mdash; in particular, whether the item has a non-empty 
     * certificate ID.
     * @function ContextOverlay.contextOverlayFilterPassed
     * @param {Uuid} entityID - The ID of the entity to check.
     * @returns {boolean} - <code>true</code> if the context overlay should be shown for the entity, <code>false</code> if it 
     *     shouldn't.
     * @example <caption>Report whether the context overlay should be displayed for entities clicked.</caption>
     * Entities.clickDownOnEntity.connect(function (id, event) {
     *     print("Item clicked:", id);
     *     print("Should display context overlay:", ContextOverlay.contextOverlayFilterPassed(id));
     * });
     */
    bool contextOverlayFilterPassed(const EntityItemID& entityItemID);

private slots:
    void handleChallengeOwnershipReplyPacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer sendingNode);

private:

    enum {
        MAX_SELECTION_COUNT = 16
    };
    bool _verboseLogging { true };
    bool _enabled { true };
    EntityItemID _mouseDownEntity;
    quint64 _mouseDownEntityTimestamp;
    EntityItemID _currentEntityWithContextOverlay;
    QString _entityMarketplaceID;
    bool _contextOverlayJustClicked { false };

    bool _isInMarketplaceInspectionMode { false };

    void enableEntityHighlight(const EntityItemID& entityItemID);
    void disableEntityHighlight(const EntityItemID& entityItemID);

    void deletingEntity(const EntityItemID& entityItemID);

    Q_INVOKABLE void startChallengeOwnershipTimer(const EntityItemID& entityItemID);
    QTimer _challengeOwnershipTimeoutTimer;
};

#endif // hifi_ContextOverlayInterface_h
