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

bool Ledger::buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const QString& buyerUsername) {
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
    return true; // FIXME send to server.
}

bool Ledger::receiveAt(const QString& hfc_key) {
    auto accountManager = DependencyManager::get<AccountManager>();
    if (!accountManager->isLoggedIn()) {
        qCWarning(commerce) << "Cannot set receiveAt when not logged in.";
        return false;
    }
    auto username = accountManager->getAccountInfo().getUsername();
    qCInfo(commerce) << "Setting default receiving key for" << username;
    return true; // FIXME send to server.
}