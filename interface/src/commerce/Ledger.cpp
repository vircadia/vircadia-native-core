//
//  Ledger.cpp
//  interface/src/commerce
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QJsonObject>
#include <QJsonDocument>
#include "AccountManager.h"
#include "Wallet.h"
#include "Ledger.h"
#include "CommerceLogging.h"

void Ledger::buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const QString& buyerUsername) {
    QJsonObject transaction;
    transaction["hfc_key"] = hfc_key;
    transaction["hfc"] = cost;
    transaction["asset_id"] = asset_id;
    transaction["inventory_key"] = inventory_key;
    transaction["inventory_buyer_username"] = buyerUsername;
    QJsonDocument transactionDoc{ transaction };
    QString transactionString = transactionDoc.toJson(QJsonDocument::Compact);
    
    auto wallet = DependencyManager::get<Wallet>();
    QString signature = wallet->signWithKey(transactionString, hfc_key);
    QJsonObject request;
    request["transaction"] = transactionString;
    request["signature"] = signature;

    qCInfo(commerce) << "Transaction:" << QJsonDocument(request).toJson(QJsonDocument::Compact);
    // FIXME: talk to server instead
    if (_inventory.contains(asset_id)) {
        // This is here more for testing than as a definition of semantics.
        // When we have popcerts, you will certainly be able to buy a new instance of an item that you already own a different instance of.
        // I'm not sure what the server should do for now in this project's MVP.
        return emit buyResult("Already owned.");
    }
    if (initializedBalance() < cost) {
        return emit buyResult("Insufficient funds.");
    }
    _balance -= cost;
    _inventory.push_back(asset_id);
    emit buyResult("");
}

bool Ledger::receiveAt(const QString& hfc_key) {
    auto accountManager = DependencyManager::get<AccountManager>();
    if (!accountManager->isLoggedIn()) {
        qCWarning(commerce) << "Cannot set receiveAt when not logged in.";
        emit receiveAtResult("Not logged in");
        return false; // We know right away that we will fail, so tell the caller.
    }
    auto username = accountManager->getAccountInfo().getUsername();
    qCInfo(commerce) << "Setting default receiving key for" << username;
    emit receiveAtResult(""); // FIXME: talk to server instead.
    return true; // Note that there may still be an asynchronous signal of failure that callers might be interested in.
}

void Ledger::balance(const QStringList& keys) {
    // FIXME: talk to server instead
    qCInfo(commerce) << "Balance:" << initializedBalance();
    emit balanceResult(_balance, "");
}

void Ledger::inventory(const QStringList& keys) {
    // FIXME: talk to server instead
    qCInfo(commerce) << "Inventory:" << _inventory;
    emit inventoryResult(_inventory, "");
}