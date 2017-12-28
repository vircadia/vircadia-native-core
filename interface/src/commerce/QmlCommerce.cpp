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

QmlCommerce::QmlCommerce() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    connect(ledger.data(), &Ledger::buyResult, this, &QmlCommerce::buyResult);
    connect(ledger.data(), &Ledger::balanceResult, this, &QmlCommerce::balanceResult);
    connect(ledger.data(), &Ledger::inventoryResult, this, &QmlCommerce::inventoryResult);
    connect(wallet.data(), &Wallet::securityImageResult, this, &QmlCommerce::securityImageResult);
    connect(ledger.data(), &Ledger::historyResult, this, &QmlCommerce::historyResult);
    connect(wallet.data(), &Wallet::keyFilePathIfExistsResult, this, &QmlCommerce::keyFilePathIfExistsResult);
    connect(ledger.data(), &Ledger::accountResult, this, &QmlCommerce::accountResult);
    connect(wallet.data(), &Wallet::walletStatusResult, this, &QmlCommerce::walletStatusResult);
    connect(ledger.data(), &Ledger::certificateInfoResult, this, &QmlCommerce::certificateInfoResult);
    connect(ledger.data(), &Ledger::updateCertificateStatus, this, &QmlCommerce::updateCertificateStatus);
    
    auto accountManager = DependencyManager::get<AccountManager>();
    connect(accountManager.data(), &AccountManager::usernameChanged, this, [&]() {
        setPassphrase("");
    });
}

void QmlCommerce::getWalletStatus() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getWalletStatus();
}

void QmlCommerce::getLoginStatus() {
    emit loginStatusResult(DependencyManager::get<AccountManager>()->isLoggedIn());
}

void QmlCommerce::getKeyFilePathIfExists() {
    auto wallet = DependencyManager::get<Wallet>();
    emit keyFilePathIfExistsResult(wallet->getKeyFilePath());
}

void QmlCommerce::getWalletAuthenticatedStatus() {
    auto wallet = DependencyManager::get<Wallet>();
    emit walletAuthenticatedStatusResult(wallet->walletIsAuthenticatedWithPassphrase());
}

void QmlCommerce::getSecurityImage() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->getSecurityImage();
}

void QmlCommerce::chooseSecurityImage(const QString& imageFile) {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->chooseSecurityImage(imageFile);
}

void QmlCommerce::buy(const QString& assetId, int cost, const bool controlledFailure) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        QJsonObject result{ { "status", "fail" }, { "message", "Uninitialized Wallet." } };
        return emit buyResult(result);
    }
    QString key = keys[0];
    // For now, we receive at the same key that pays for it.
    ledger->buy(key, cost, assetId, key, controlledFailure);
}

void QmlCommerce::balance() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->balance(cachedPublicKeys);
    }
}

void QmlCommerce::inventory() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->inventory(cachedPublicKeys);
    }
}

void QmlCommerce::history() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        ledger->history(cachedPublicKeys);
    }
}

void QmlCommerce::changePassphrase(const QString& oldPassphrase, const QString& newPassphrase) {
    auto wallet = DependencyManager::get<Wallet>();
    if (wallet->getPassphrase()->isEmpty()) {
        emit changePassphraseStatusResult(wallet->setPassphrase(newPassphrase));
    } else if (wallet->getPassphrase() == oldPassphrase && !newPassphrase.isEmpty()) {
        emit changePassphraseStatusResult(wallet->changePassphrase(newPassphrase));
    } else {
        emit changePassphraseStatusResult(false);
    }
}

void QmlCommerce::setPassphrase(const QString& passphrase) {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->setPassphrase(passphrase);
    getWalletAuthenticatedStatus();
}

void QmlCommerce::generateKeyPair() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->generateKeyPair();
    getWalletAuthenticatedStatus();
}

void QmlCommerce::reset() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    ledger->reset();
    wallet->reset();
}

void QmlCommerce::resetLocalWalletOnly() {
    auto wallet = DependencyManager::get<Wallet>();
    wallet->reset();
}

void QmlCommerce::account() {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->account();
}

void QmlCommerce::certificateInfo(const QString& certificateId) {
    auto ledger = DependencyManager::get<Ledger>();
    ledger->certificateInfo(certificateId);
}
