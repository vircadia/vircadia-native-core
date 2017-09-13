//
//  Ledger.h
//  interface/src/commerce
//
// Bottlenecks all interaction with the blockchain or other ledger system.
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Ledger_h
#define hifi_Ledger_h

#include <QJsonObject>
#include <DependencyManager.h>
#include <QtNetwork/QNetworkReply>


class Ledger : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const QString& buyerUsername = "");
    bool receiveAt(const QString& hfc_key, const QString& old_key);
    void balance(const QStringList& keys);
    void inventory(const QStringList& keys);
    void history(const QStringList& keys);
    void account();
    void reset();

signals:
    void buyResult(QJsonObject result);
    void receiveAtResult(QJsonObject result);
    void balanceResult(QJsonObject result);
    void inventoryResult(QJsonObject result);
    void historyResult(QJsonObject result);
    void accountResult(QJsonObject result);

public slots:
    void buySuccess(QNetworkReply& reply);
    void buyFailure(QNetworkReply& reply);
    void receiveAtSuccess(QNetworkReply& reply);
    void receiveAtFailure(QNetworkReply& reply);
    void balanceSuccess(QNetworkReply& reply);
    void balanceFailure(QNetworkReply& reply);
    void inventorySuccess(QNetworkReply& reply);
    void inventoryFailure(QNetworkReply& reply);
    void historySuccess(QNetworkReply& reply);
    void historyFailure(QNetworkReply& reply);
    void resetSuccess(QNetworkReply& reply);
    void resetFailure(QNetworkReply& reply);
    void accountSuccess(QNetworkReply& reply);
    void accountFailure(QNetworkReply& reply);

private:
    QJsonObject apiResponse(const QString& label, QNetworkReply& reply);
    QJsonObject failResponse(const QString& label, QNetworkReply& reply);
    void send(const QString& endpoint, const QString& success, const QString& fail, QNetworkAccessManager::Operation method, QJsonObject request);
    void keysQuery(const QString& endpoint, const QString& success, const QString& fail);
    void signedSend(const QString& propertyName, const QByteArray& text, const QString& key, const QString& endpoint, const QString& success, const QString& fail);
};

#endif // hifi_Ledger_h
