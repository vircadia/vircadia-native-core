//
//  WalletScriptingInterface.h
//  interface/src/scripting
//
//  Created by Zach Fox on 2017-09-29.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_WalletScriptingInterface_h
#define hifi_WalletScriptingInterface_h

#include <QtCore/QObject>
#include <DependencyManager.h>

#include <ui/TabletScriptingInterface.h>
#include <ui/QmlWrapper.h>
#include <OffscreenUi.h>
#include "Application.h"
#include "commerce/Wallet.h"
#include "ui/overlays/ContextOverlayInterface.h"
#include <AccountManager.h>

class CheckoutProxy : public QmlWrapper {
    Q_OBJECT
public:
    CheckoutProxy(QObject* qmlObject, QObject* parent = nullptr);
};

/*@jsdoc
 * The <code>WalletScriptingInterface</code> API provides functions related to the user's wallet and verification of certified 
 * avatar entities.
 *
 * @namespace WalletScriptingInterface
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {WalletScriptingInterface.WalletStatus} walletStatus - The status of the user's wallet. <em>Read-only.</em>
 * @property {boolean} limitedCommerce - <code>true</code> if Interface is running in limited commerce mode. In limited commerce 
 *     mode, certain Interface functionalities are disabled, e.g., users can't buy items that are not free from the Marketplace. 
 *     The Oculus Store and Steam versions of Interface run in limited commerce mode. <em>Read-only.</em>
 */
class WalletScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(uint walletStatus READ getWalletStatus WRITE setWalletStatus NOTIFY walletStatusChanged)
    Q_PROPERTY(bool limitedCommerce READ getLimitedCommerce WRITE setLimitedCommerce NOTIFY limitedCommerceChanged)

public:

    WalletScriptingInterface();

    /*@jsdoc
     * Checks and updates the user's wallet status.
     * @function WalletScriptingInterface.refreshWalletStatus
     */
    Q_INVOKABLE void refreshWalletStatus();

    /*@jsdoc
     * Gets the current status of the user's wallet.
     * @function WalletScriptingInterface.getWalletStatus
     * @returns {WalletScriptingInterface.WalletStatus}
     * @example <caption>Use two methods to report your wallet's status.</caption>
     * print("Wallet status: " + WalletScriptingInterface.walletStatus);  // Same value as next line.
     * print("Wallet status: " + WalletScriptingInterface.getWalletStatus());
     */
    Q_INVOKABLE uint getWalletStatus() { return _walletStatus; }

    /*@jsdoc
     * Check that a certified avatar entity is owned by the avatar whose entity it is. The result of the check is provided via 
     * the {@link WalletScriptingInterface.ownershipVerificationSuccess|ownershipVerificationSuccess} and 
     * {@link WalletScriptingInterface.ownershipVerificationFailed|ownershipVerificationFailed} signals.
     * <p><strong>Warning:</strong> Neither of these signals are triggered if the entity is not an avatar entity or is not 
     * certified.</p>
     * @function WalletScriptingInterface.proveAvatarEntityOwnershipVerification
     * @param {Uuid} entityID - The avatar entity's ID.
     * @example <caption>Check the ownership of all nearby certified avatar entities.</caption>
     * // Set up response handling.
     * function ownershipSuccess(entityID) {
     *     print("Ownership test succeeded for: " + entityID);
     * }
     * function ownershipFailed(entityID) {
     *     print("Ownership test failed for: " + entityID);
     * }
     * WalletScriptingInterface.ownershipVerificationSuccess.connect(ownershipSuccess);
     * WalletScriptingInterface.ownershipVerificationFailed.connect(ownershipFailed);
     * 
     * // Check ownership of all nearby certified avatar entities.
     * var entityIDs = Entities.findEntities(MyAvatar.position, 10);
     * var i, length;
     * for (i = 0, length = entityIDs.length; i < length; i++) {
     *     var properties = Entities.getEntityProperties(entityIDs[i], ["entityHostType", "certificateID"]);
     *     if (properties.entityHostType === "avatar" && properties.certificateID !== "") {
     *         print("Prove ownership of: " + entityIDs[i]);
     *         WalletScriptingInterface.proveAvatarEntityOwnershipVerification(entityIDs[i]);
     *     }
     * }
     * 
     * // Tidy up.
     * Script.scriptEnding.connect(function () {
     *     WalletScriptingInterface.ownershipVerificationFailed.disconnect(ownershipFailed);
     *     WalletScriptingInterface.ownershipVerificationSuccess.disconnect(ownershipSuccess);
     * });
     */
    Q_INVOKABLE void proveAvatarEntityOwnershipVerification(const QUuid& entityID);

    // setWalletStatus() should never be made Q_INVOKABLE. If it were,
    //     scripts could cause the Wallet to incorrectly report its status.
    void setWalletStatus(const uint& status);

    bool getLimitedCommerce() { return DependencyManager::get<AccountManager>()->getLimitedCommerce(); }
    void setLimitedCommerce(bool isLimited) { DependencyManager::get<AccountManager>()->setLimitedCommerce(isLimited); };

signals:

    /*@jsdoc
     * Triggered when the user's wallet status changes.
     * @function WalletScriptingInterface.walletStatusChanged
     * @returns {Signal}
     * @example <caption>Report when your wallet status changes, e.g., when you log in and out.</caption>
     * WalletScriptingInterface.walletStatusChanged.connect(function () {
     *     print("Wallet status changed to: " + WalletScriptingInterface.walletStatus");
     * });
     */
    void walletStatusChanged();

    /*@jsdoc
     * Triggered when the user's limited commerce status changes.
     * @function WalletScriptingInterface.limitedCommerceChanged
     * @returns {Signal}
     */
    void limitedCommerceChanged();

    /*@jsdoc
     * Triggered when the user rezzes a certified entity but the user's wallet is not ready. So the certified location of the
     * entity cannot be updated in the metaverse.
     * @function WalletScriptingInterface.walletNotSetup
     * @returns {Signal}
     */
    void walletNotSetup();

    /*@jsdoc
     * Triggered when a certified avatar entity's ownership check requested via 
     * {@link WalletScriptingInterface.proveAvatarEntityOwnershipVerification|proveAvatarEntityOwnershipVerification} or 
     * {@link ContextOverlay.requestOwnershipVerification} succeeds. 
     * @function WalletScriptingInterface.ownershipVerificationSuccess
     * @param {Uuid} entityID - The ID of the avatar entity checked.
     * @returns {Signal}
     */
    void ownershipVerificationSuccess(const QUuid& entityID);

    /*@jsdoc
     * Triggered when a certified avatar entity's ownership check requested via
     * {@link WalletScriptingInterface.proveAvatarEntityOwnershipVerification|proveAvatarEntityOwnershipVerification} or
     * {@link ContextOverlay.requestOwnershipVerification} fails.
     * @function WalletScriptingInterface.ownershipVerificationFailed
     * @param {Uuid} entityID - The ID of the avatar entity checked.
     * @returns {Signal}
     */
    void ownershipVerificationFailed(const QUuid& entityID);

private:
    uint _walletStatus;
};

#endif // hifi_WalletScriptingInterface_h
