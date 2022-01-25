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
#include <QtCore/QSharedPointer>
#include <SettingHandle.h>

CheckoutProxy::CheckoutProxy(QObject* qmlObject, QObject* parent) : QmlWrapper(qmlObject, parent) {
    Q_ASSERT(QThread::currentThread() == qApp->thread());
}

WalletScriptingInterface::WalletScriptingInterface() {
    connect(DependencyManager::get<AccountManager>().data(),
        &AccountManager::limitedCommerceChanged, this, &WalletScriptingInterface::limitedCommerceChanged);
}

void WalletScriptingInterface::refreshWalletStatus() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getWalletStatus();
}

void WalletScriptingInterface::setWalletStatus(const uint& status) {
    _walletStatus = status;
    emit DependencyManager::get<Wallet>()->walletStatusResult(status);
}

void WalletScriptingInterface::proveAvatarEntityOwnershipVerification(const QUuid& entityID) {
    QSharedPointer<ContextOverlayInterface> contextOverlayInterface = DependencyManager::get<ContextOverlayInterface>();
    EntityItemProperties entityProperties = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(entityID,
        contextOverlayInterface->getEntityPropertyFlags());
    if (entityProperties.getEntityHostType() == entity::HostType::AVATAR) {
        if (!entityID.isNull() && entityProperties.getCertificateID().length() > 0) {
            contextOverlayInterface->requestOwnershipVerification(entityID);
        } else {
            qCDebug(entities) << "Failed to prove ownership of:" << entityID << "is null or not a certified item";
        }
    } else {
        qCDebug(entities) << "Failed to prove ownership of:" << entityID << "is not an avatar entity";
    }
}
