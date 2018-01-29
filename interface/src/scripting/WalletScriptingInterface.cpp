//
//  WalletScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Zach Fox on 2017-09-29.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "WalletScriptingInterface.h"

CheckoutProxy::CheckoutProxy(QObject* qmlObject, QObject* parent) : QmlWrapper(qmlObject, parent) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

WalletScriptingInterface::WalletScriptingInterface() {

    _entityPropertyFlags += PROP_POSITION;
    _entityPropertyFlags += PROP_ROTATION;
    _entityPropertyFlags += PROP_MARKETPLACE_ID;
    _entityPropertyFlags += PROP_DIMENSIONS;
    _entityPropertyFlags += PROP_REGISTRATION_POINT;
    _entityPropertyFlags += PROP_CERTIFICATE_ID;
    _entityPropertyFlags += PROP_CLIENT_ONLY;
    _entityPropertyFlags += PROP_OWNING_AVATAR_ID;
}

void WalletScriptingInterface::refreshWalletStatus() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getWalletStatus();
}

void WalletScriptingInterface::setWalletStatus(const uint& status) {
    _walletStatus = status;
    emit DependencyManager::get<Wallet>()->walletStatusResult(status);
}

void WalletScriptingInterface::proveEntityOwnershipVerification(const QUuid& entityID) {
    EntityItemProperties entityProperties = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID, _entityPropertyFlags);
    if (!entityID.isNull() && entityProperties.getMarketplaceID().length() > 0) {
        DependencyManager::get<ContextOverlayInterface>()->requestOwnershipVerification(entityID);
    } else {
        qCDebug(entities) << "Failed to prove ownership of:" << entityID << "is null or not a marketplace item";
    }
}