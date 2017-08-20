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
#include <QJsonArray>
#include <QJsonDocument>
#include "AccountManager.h"
#include "Wallet.h"
#include "Ledger.h"
#include "CommerceLogging.h"

// inventory answers {status: 'success', data: {assets: [{id: "guid", title: "name", preview: "url"}....]}}
// balance answers {status: 'success', data: {balance: integer}}
// buy and receive_at answer {status: 'success'}

QJsonObject Ledger::apiResponse(const QString& label, QNetworkReply& reply) {
    QByteArray response = reply.readAll();
    QJsonObject data = QJsonDocument::fromJson(response).object();
    qInfo(commerce) << label << "response" << QJsonDocument(data).toJson(QJsonDocument::Compact);
    return data;
}
// Non-200 responses are not json:
QJsonObject Ledger::failResponse(const QString& label, QNetworkReply& reply) {
    QString response = reply.readAll();
    qWarning(commerce) << "FAILED" << label << response;
    QJsonObject result
    {
        { "status", "fail" },
        { "message", response }
    };
    return result;
}
#define ApiHandler(NAME) void Ledger::NAME##Success(QNetworkReply& reply) { emit NAME##Result(apiResponse(#NAME, reply)); }
#define FailHandler(NAME) void Ledger::NAME##Failure(QNetworkReply& reply) { emit NAME##Result(failResponse(#NAME, reply)); }
#define Handler(NAME) ApiHandler(NAME) FailHandler(NAME)
Handler(buy)
Handler(receiveAt)
Handler(balance)
Handler(inventory)

void Ledger::send(const QString& endpoint, const QString& success, const QString& fail, QNetworkAccessManager::Operation method, QJsonObject request) {
    auto accountManager = DependencyManager::get<AccountManager>();
    const QString URL = "/api/v1/commerce/";
    JSONCallbackParameters callbackParams(this, success, this, fail);
    qCInfo(commerce) << "Sending" << endpoint << QJsonDocument(request).toJson(QJsonDocument::Compact);
    accountManager->sendRequest(URL + endpoint,
        AccountManagerAuth::Required,
        method,
        callbackParams,
        QJsonDocument(request).toJson());
}

void Ledger::signedSend(const QString& propertyName, const QByteArray& text, const QString& key, const QString& endpoint, const QString& success, const QString& fail) {
    auto wallet = DependencyManager::get<Wallet>();
    QString signature = key.isEmpty() ? "" : wallet->signWithKey(text, key);
    QJsonObject request;
    request[propertyName] = QString(text);
    request["signature"] = signature;
    send(endpoint, success, fail, QNetworkAccessManager::PutOperation, request);
}

void Ledger::keysQuery(const QString& endpoint, const QString& success, const QString& fail) {
    auto wallet = DependencyManager::get<Wallet>();
    QJsonObject request;
    request["public_keys"] = QJsonArray::fromStringList(wallet->listPublicKeys());
    send(endpoint, success, fail, QNetworkAccessManager::PostOperation, request);
}

void Ledger::buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const QString& buyerUsername) {
    QJsonObject transaction;
    transaction["hfc_key"] = hfc_key;
    transaction["cost"] = cost;
    transaction["asset_id"] = asset_id;
    transaction["inventory_key"] = inventory_key;
    transaction["inventory_buyer_username"] = buyerUsername;
    QJsonDocument transactionDoc{ transaction };
    auto transactionString = transactionDoc.toJson(QJsonDocument::Compact);
    signedSend("transaction", transactionString, hfc_key, "buy", "buySuccess", "buyFailure");
}

bool Ledger::receiveAt(const QString& hfc_key, const QString& old_key) {
    auto accountManager = DependencyManager::get<AccountManager>();
    if (!accountManager->isLoggedIn()) {
        qCWarning(commerce) << "Cannot set receiveAt when not logged in.";
        QJsonObject result{ { "status", "fail" }, { "message", "Not logged in" } };
        emit receiveAtResult(result);
        return false; // We know right away that we will fail, so tell the caller.
    }

    signedSend("public_key", hfc_key.toUtf8(), old_key, "receive_at", "receiveAtSuccess", "receiveAtFailure");
    return true; // Note that there may still be an asynchronous signal of failure that callers might be interested in.
}

void Ledger::balance(const QStringList& keys) {
    keysQuery("balance", "balanceSuccess", "balanceFailure");
}

void Ledger::inventory(const QStringList& keys) {
    keysQuery("inventory", "inventorySuccess", "inventoryFailure");
}

