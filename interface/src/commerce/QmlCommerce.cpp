//
//  QmlCommerce.cpp
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
#include <AccountManager.h>

HIFI_QML_DEF(QmlCommerce)

QmlCommerce::QmlCommerce(QQuickItem* parent) : OffscreenQmlDialog(parent) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    connect(ledger.data(), &Ledger::buyResult, this, &QmlCommerce::buyResult);
    connect(ledger.data(), &Ledger::balanceResult, this, &QmlCommerce::balanceResult);
    connect(ledger.data(), &Ledger::inventoryResult, this, &QmlCommerce::inventoryResult);
    connect(wallet.data(), &Wallet::securityImageResult, this, &QmlCommerce::securityImageResult);
    connect(wallet.data(), &Wallet::keyFilePathResult, this, &QmlCommerce::keyFilePathResult);
}

void QmlCommerce::buy(const QString& assetId, int cost, const QString& buyerUsername) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" }, { "message", "Uninitialized Wallet." } };
        return emit buyResult(result);
    }
    QString key = keys[0];
    // For now, we receive at the same key that pays for it.
    ledger->buy(key, cost, assetId, key, buyerUsername);
}

void QmlCommerce::balance() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    ledger->balance(wallet->listPublicKeys());
}
void QmlCommerce::inventory() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    ledger->inventory(wallet->listPublicKeys());
}

void QmlCommerce::chooseSecurityImage(const QString& imageFile) {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->chooseSecurityImage(imageFile);
}
void QmlCommerce::getSecurityImage() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getSecurityImage();
}
void QmlCommerce::getLoginStatus() {
    emit loginStatusResult(DependencyManager::get<AccountManager>()->isLoggedIn());
}
void QmlCommerce::setPassphrase(const QString& passphrase) {
    emit passphraseSetupStatusResult(true);
}
void QmlCommerce::getPassphraseSetupStatus() {
    emit passphraseSetupStatusResult(false);
}
void QmlCommerce::getKeyFilePath() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getKeyFilePath();
}
