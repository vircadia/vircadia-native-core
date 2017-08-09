//
//  Commerce.cpp
//  interface/src/commerce
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlCommerce.h"
#include "Application.h"
#include "DependencyManager.h"
#include "Ledger.h"
#include "Wallet.h"

HIFI_QML_DEF(QmlCommerce)

bool QmlCommerce::buy(const QString& assetId, int cost, const QString& buyerUsername) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        return false;
    }
    QString key = keys[0];
    // For now, we receive at the same key that pays for it.
    return ledger->buy(key, cost, assetId, key, buyerUsername);
}
