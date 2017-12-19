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
}

void WalletScriptingInterface::refreshWalletStatus() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getWalletStatus();
}

void WalletScriptingInterface::setWalletStatus(const uint& status) {
    _walletStatus = status;
    emit DependencyManager::get<Wallet>()->walletStatusResult(status);
}