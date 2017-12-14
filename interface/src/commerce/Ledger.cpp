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
#include <QTimeZone>
#include <QJsonDocument>
#include "Wallet.h"
#include "Ledger.h"
#include "CommerceLogging.h"
#include <NetworkingConstants.h>

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

void Ledger::send(const QString& endpoint, const QString& success, const QString& fail, QNetworkAccessManager::Operation method, AccountManagerAuth::Type authType, QJsonObject request) {
    auto accountManager = DependencyManager::get<AccountManager>();
    const QString URL = "/api/v1/commerce/";
    JSONCallbackParameters callbackParams(this, success, this, fail);
    qCInfo(commerce) << "Sending" << endpoint << QJsonDocument(request).toJson(QJsonDocument::Compact);
    accountManager->sendRequest(URL + endpoint,
        authType,
        method,
        callbackParams,
        QJsonDocument(request).toJson());
}

void Ledger::signedSend(const QString& propertyName, const QByteArray& text, const QString& key, const QString& endpoint, const QString& success, const QString& fail, const bool controlled_failure) {
    auto wallet = DependencyManager::get<Wallet>();
    QString signature = key.isEmpty() ? "" : wallet->signWithKey(text, key);
    QJsonObject request;
    request[propertyName] = QString(text);
    if (!controlled_failure) {
        request["signature"] = signature;
    } else {
        request["signature"] = QString("controlled failure!");
    }
    send(endpoint, success, fail, QNetworkAccessManager::PutOperation, AccountManagerAuth::Required, request);
}

void Ledger::keysQuery(const QString& endpoint, const QString& success, const QString& fail) {
    auto wallet = DependencyManager::get<Wallet>();
    QJsonObject request;
    request["public_keys"] = QJsonArray::fromStringList(wallet->listPublicKeys());
    send(endpoint, success, fail, QNetworkAccessManager::PostOperation, AccountManagerAuth::Required, request);
}

void Ledger::buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const bool controlled_failure) {
    QJsonObject transaction;
    transaction["hfc_key"] = hfc_key;
    transaction["cost"] = cost;
    transaction["asset_id"] = asset_id;
    transaction["inventory_key"] = inventory_key;
    QJsonDocument transactionDoc{ transaction };
    auto transactionString = transactionDoc.toJson(QJsonDocument::Compact);
    signedSend("transaction", transactionString, hfc_key, "buy", "buySuccess", "buyFailure", controlled_failure);
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

QString amountString(const QString& label, const QString&color, const QJsonValue& moneyValue, const QJsonValue& certsValue) {
    int money = moneyValue.toInt();
    int certs = certsValue.toInt();
    if (money <= 0 && certs <= 0) {
        return QString();
    }
    QString result(QString("<font color='#%1'> %2").arg(color, label));
    if (money > 0) {
        result += QString(" %1 HFC").arg(money);
    }
    if (certs > 0) {
        if (money > 0) {
            result += QString(",");
        }
        result += QString((certs == 1) ? " %1 certificate" : " %1 certificates").arg(certs);
    }
    return result + QString("</font>");
}

static const QString MARKETPLACE_ITEMS_BASE_URL = NetworkingConstants::METAVERSE_SERVER_URL().toString() + "/marketplace/items/";
void Ledger::historySuccess(QNetworkReply& reply) {
    // here we send a historyResult with some extra stuff in it
    // Namely, the styled text we'd like to show.  The issue is the
    // QML cannot do that easily since it doesn't know what the wallet
    // public key(s) are.  Let's keep it that way
    QByteArray response = reply.readAll();
    QJsonObject data = QJsonDocument::fromJson(response).object();
    qInfo(commerce) << "history" << "response" << QJsonDocument(data).toJson(QJsonDocument::Compact);

    // we will need the array of public keys from the wallet
    auto wallet = DependencyManager::get<Wallet>();
    auto keys = wallet->listPublicKeys();

    // now we need to loop through the transactions and add fancy text...
    auto historyArray = data.find("data").value().toObject().find("history").value().toArray();
    QJsonArray newHistoryArray;

    // TODO: do this with 0 copies if possible
    for (auto it = historyArray.begin(); it != historyArray.end(); it++) {
        auto valueObject = (*it).toObject();
        QString sent = amountString("sent", "EA4C5F", valueObject["sent_money"], valueObject["sent_certs"]);
        QString received = amountString("received", "1FC6A6", valueObject["received_money"], valueObject["received_certs"]);

        // turns out on my machine, toLocalTime convert to some weird timezone, yet the
        // systemTimeZone is correct.  To avoid a strange bug with other's systems too, lets
        // be explicit
        QDateTime createdAt = QDateTime::fromSecsSinceEpoch(valueObject["created_at"].toInt(), Qt::UTC);
        QDateTime localCreatedAt = createdAt.toTimeZone(QTimeZone::systemTimeZone());
        valueObject["text"] = QString("%1%2%3").arg(valueObject["message"].toString(), sent, received);
        newHistoryArray.push_back(valueObject);
    }
    // now copy the rest of the json -- this is inefficient
    // TODO: try to do this without making copies
    QJsonObject newData;
    newData["status"] = "success";
    QJsonObject newDataData;
    newDataData["history"] = newHistoryArray;
    newData["data"] = newDataData;
    emit historyResult(newData);
}

void Ledger::historyFailure(QNetworkReply& reply) {
    failResponse("history", reply);
}

void Ledger::history(const QStringList& keys) {
    keysQuery("history", "historySuccess", "historyFailure");
}

// The api/failResponse is called just for the side effect of logging.
void Ledger::resetSuccess(QNetworkReply& reply) { apiResponse("reset", reply); }
void Ledger::resetFailure(QNetworkReply& reply) { failResponse("reset", reply); }
void Ledger::reset() {
    send("reset_user_hfc_account", "resetSuccess", "resetFailure", QNetworkAccessManager::PutOperation, AccountManagerAuth::Required, QJsonObject());
}

void Ledger::accountSuccess(QNetworkReply& reply) {
    // lets set the appropriate stuff in the wallet now
    auto wallet = DependencyManager::get<Wallet>();
    QByteArray response = reply.readAll();
    QJsonObject data = QJsonDocument::fromJson(response).object()["data"].toObject();

    auto salt = QByteArray::fromBase64(data["salt"].toString().toUtf8());
    auto iv = QByteArray::fromBase64(data["iv"].toString().toUtf8());
    auto ckey = QByteArray::fromBase64(data["ckey"].toString().toUtf8());

    wallet->setSalt(salt);
    wallet->setIv(iv);
    wallet->setCKey(ckey);

    // none of the hfc account info should be emitted
    emit accountResult(QJsonObject{ {"status", "success"} });
}

void Ledger::accountFailure(QNetworkReply& reply) {
    failResponse("account", reply);
}
void Ledger::account() {
    send("hfc_account", "accountSuccess", "accountFailure", QNetworkAccessManager::PutOperation, AccountManagerAuth::Required, QJsonObject());
}

// The api/failResponse is called just for the side effect of logging.
void Ledger::updateLocationSuccess(QNetworkReply& reply) { apiResponse("updateLocation", reply); }
void Ledger::updateLocationFailure(QNetworkReply& reply) { failResponse("updateLocation", reply); }
void Ledger::updateLocation(const QString& asset_id, const QString location, const bool controlledFailure) {
    auto wallet = DependencyManager::get<Wallet>();
    auto walletScriptingInterface = DependencyManager::get<WalletScriptingInterface>();
    uint walletStatus = walletScriptingInterface->getWalletStatus();

    if (walletStatus != (uint)wallet->WALLET_STATUS_READY) {
        emit walletScriptingInterface->walletNotSetup();
        qDebug(commerce) << "User attempted to update the location of a certificate, but their wallet wasn't ready. Status:" << walletStatus;
    } else {
        QStringList keys = wallet->listPublicKeys();
        QString key = keys[0];
        QJsonObject transaction;
        transaction["certificate_id"] = asset_id;
        transaction["place_name"] = location;
        QJsonDocument transactionDoc{ transaction };
        auto transactionString = transactionDoc.toJson(QJsonDocument::Compact);
        signedSend("transaction", transactionString, key, "location", "updateLocationSuccess", "updateLocationFailure", controlledFailure);
    }
}

void Ledger::certificateInfoSuccess(QNetworkReply& reply) {
    auto wallet = DependencyManager::get<Wallet>();
    auto accountManager = DependencyManager::get<AccountManager>();

    QByteArray response = reply.readAll();
    QJsonObject replyObject = QJsonDocument::fromJson(response).object();

    QStringList keys = wallet->listPublicKeys();
    if (keys.count() != 0) {
        QJsonObject data = replyObject["data"].toObject();
        if (data["transfer_recipient_key"].toString() == keys[0]) {
            replyObject.insert("isMyCert", true);
        }
    }
    qInfo(commerce) << "certificateInfo" << "response" << QJsonDocument(replyObject).toJson(QJsonDocument::Compact);
    emit certificateInfoResult(replyObject);
}
void Ledger::certificateInfoFailure(QNetworkReply& reply) { failResponse("certificateInfo", reply); }
void Ledger::certificateInfo(const QString& certificateId) {
    QString endpoint = "proof_of_purchase_status/transfer";
    QJsonObject request;
    request["certificate_id"] = certificateId;
    send(endpoint, "certificateInfoSuccess", "certificateInfoFailure", QNetworkAccessManager::PutOperation, AccountManagerAuth::None, request);
}
