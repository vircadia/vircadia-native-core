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
#include "AccountManager.h"


class Ledger : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

public:
    void buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const bool controlled_failure = false);
    bool receiveAt(const QString& hfc_key, const QString& signing_key, const QByteArray& locker);
    bool receiveAt();
    void balance(const QStringList& keys);
    void inventory(const QString& editionFilter, const QString& typeFilter, const QString& titleFilter, const int& page, const int& perPage);
    void history(const QStringList& keys, const int& pageNumber, const int& itemsPerPage);
    void account();
    void updateLocation(const QString& asset_id, const QString& location, const bool& alsoUpdateSiblings = false, const bool controlledFailure = false);
    void certificateInfo(const QString& certificateId);
    void transferAssetToNode(const QString& hfc_key, const QString& nodeID, const QString& certificateID, const int& amount, const QString& optionalMessage);
    void transferAssetToUsername(const QString& hfc_key, const QString& username, const QString& certificateID, const int& amount, const QString& optionalMessage);
    void alreadyOwned(const QString& marketplaceId);
    void getAvailableUpdates(const QString& itemId = "");
    void updateItem(const QString& hfc_key, const QString& certificate_id);

    enum CertificateStatus {
        CERTIFICATE_STATUS_UNKNOWN = 0,
        CERTIFICATE_STATUS_VERIFICATION_SUCCESS,
        CERTIFICATE_STATUS_VERIFICATION_TIMEOUT,
        CERTIFICATE_STATUS_STATIC_VERIFICATION_FAILED,
        CERTIFICATE_STATUS_OWNER_VERIFICATION_FAILED,
    };

signals:
    void buyResult(QJsonObject result);
    void receiveAtResult(QJsonObject result);
    void balanceResult(QJsonObject result);
    void inventoryResult(QJsonObject result);
    void historyResult(QJsonObject result);
    void accountResult(QJsonObject result);
    void locationUpdateResult(QJsonObject result);
    void certificateInfoResult(QJsonObject result);
    void transferAssetToNodeResult(QJsonObject result);
    void transferAssetToUsernameResult(QJsonObject result);
    void alreadyOwnedResult(QJsonObject result);
    void availableUpdatesResult(QJsonObject result);
    void updateItemResult(QJsonObject result);

    void updateCertificateStatus(const QString& certID, uint certStatus);

public slots:
    void buySuccess(QNetworkReply* reply);
    void buyFailure(QNetworkReply* reply);
    void receiveAtSuccess(QNetworkReply* reply);
    void receiveAtFailure(QNetworkReply* reply);
    void balanceSuccess(QNetworkReply* reply);
    void balanceFailure(QNetworkReply* reply);
    void inventorySuccess(QNetworkReply* reply);
    void inventoryFailure(QNetworkReply* reply);
    void historySuccess(QNetworkReply* reply);
    void historyFailure(QNetworkReply* reply);
    void accountSuccess(QNetworkReply* reply);
    void accountFailure(QNetworkReply* reply);
    void updateLocationSuccess(QNetworkReply* reply);
    void updateLocationFailure(QNetworkReply* reply);
    void certificateInfoSuccess(QNetworkReply* reply);
    void certificateInfoFailure(QNetworkReply* reply);
    void transferAssetToNodeSuccess(QNetworkReply* reply);
    void transferAssetToNodeFailure(QNetworkReply* reply);
    void transferAssetToUsernameSuccess(QNetworkReply* reply);
    void transferAssetToUsernameFailure(QNetworkReply* reply);
    void alreadyOwnedSuccess(QNetworkReply* reply);
    void alreadyOwnedFailure(QNetworkReply* reply);
    void availableUpdatesSuccess(QNetworkReply* reply);
    void availableUpdatesFailure(QNetworkReply* reply);
    void updateItemSuccess(QNetworkReply* reply);
    void updateItemFailure(QNetworkReply* reply);

private:
    QJsonObject apiResponse(const QString& label, QNetworkReply* reply);
    QJsonObject failResponse(const QString& label, QNetworkReply* reply);
    void send(const QString& endpoint, const QString& success, const QString& fail, QNetworkAccessManager::Operation method, AccountManagerAuth::Type authType, QJsonObject request);
    void keysQuery(const QString& endpoint, const QString& success, const QString& fail, QJsonObject& extraRequestParams);
    void keysQuery(const QString& endpoint, const QString& success, const QString& fail);
    void signedSend(const QString& propertyName, const QByteArray& text, const QString& key, const QString& endpoint, const QString& success, const QString& fail, const bool controlled_failure = false);
};

#endif // hifi_Ledger_h
