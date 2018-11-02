
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

/**jsdoc
 * @namespace Wallet
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {number} walletStatus
 * @property {bool} limitedCommerce
 */
class WalletScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_PROPERTY(uint walletStatus READ getWalletStatus WRITE setWalletStatus NOTIFY walletStatusChanged)
    Q_PROPERTY(bool limitedCommerce READ getLimitedCommerce WRITE setLimitedCommerce NOTIFY limitedCommerceChanged)

public:

    WalletScriptingInterface();

    /**jsdoc
     * @function WalletScriptingInterface.refreshWalletStatus
     */
    Q_INVOKABLE void refreshWalletStatus();

    /**jsdoc
     * @function WalletScriptingInterface.getWalletStatus
     * @returns {number}
     */
    Q_INVOKABLE uint getWalletStatus() { return _walletStatus; }

    /**jsdoc
     * @function WalletScriptingInterface.proveAvatarEntityOwnershipVerification
     * @param {Uuid} entityID
     */
    Q_INVOKABLE void proveAvatarEntityOwnershipVerification(const QUuid& entityID);

    // setWalletStatus() should never be made Q_INVOKABLE. If it were,
    //     scripts could cause the Wallet to incorrectly report its status.
    void setWalletStatus(const uint& status);

    bool getLimitedCommerce() { return DependencyManager::get<AccountManager>()->getLimitedCommerce(); }
    void setLimitedCommerce(bool isLimited) { DependencyManager::get<AccountManager>()->setLimitedCommerce(isLimited); };

signals:

    /**jsdoc
     * @function WalletScriptingInterface.walletStatusChanged
     * @returns {Signal}
     */
    void walletStatusChanged();

    /**jsdoc
     * @function WalletScriptingInterface.limitedCommerceChanged
     * @returns {Signal}
     */
    void limitedCommerceChanged();

    /**jsdoc
     * @function WalletScriptingInterface.walletNotSetup
     * @returns {Signal}
     */
    void walletNotSetup();

    /**jsdoc
     * @function WalletScriptingInterface.ownershipVerificationSuccess
     * @param {Uuid} entityID
     * @returns {Signal}
     */
    void ownershipVerificationSuccess(const QUuid& entityID);

    /**jsdoc
     * @function WalletScriptingInterface.ownershipVerificationFailed
     * @param {Uuid} entityID
     * @returns {Signal}
     */
    void ownershipVerificationFailed(const QUuid& entityID);

private:
    uint _walletStatus;
};

#endif // hifi_WalletScriptingInterface_h
