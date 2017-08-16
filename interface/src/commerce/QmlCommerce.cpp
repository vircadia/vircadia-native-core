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
    bool success = ledger->buy(key, cost, assetId, key, buyerUsername);
    // FIXME: until we start talking to server, report post-transaction balance and inventory so we can see log for testing.
    balance();
    inventory();
    return success;
}

int QmlCommerce::balance() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    return ledger->balance(wallet->listPublicKeys());
}
QStringList QmlCommerce::inventory() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    return ledger->inventory(wallet->listPublicKeys());
 }