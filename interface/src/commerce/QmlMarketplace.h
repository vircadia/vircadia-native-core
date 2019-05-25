//
//  QmlMarketplace.h
//  interface/src/commerce
//
//  Guard for safe use of Marketplace by authorized QML.
//
//  Created by Roxanne Skelly on 1/18/19.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_QmlMarketplace_h
#define hifi_QmlMarketplace_h

#include <QJsonObject>

#include <QPixmap>
#include <QtNetwork/QNetworkReply>
#include "AccountManager.h"

class QmlMarketplace : public QObject {
    Q_OBJECT

public:
    QmlMarketplace();

public slots:
    void getMarketplaceItemsSuccess(QNetworkReply* reply);
    void getMarketplaceItemsFailure(QNetworkReply* reply);
    void getMarketplaceItemSuccess(QNetworkReply* reply);
    void getMarketplaceItemFailure(QNetworkReply* reply);
    void getMarketplaceCategoriesSuccess(QNetworkReply* reply);
    void getMarketplaceCategoriesFailure(QNetworkReply* reply);
    void marketplaceItemLikeSuccess(QNetworkReply* reply);
    void marketplaceItemLikeFailure(QNetworkReply* reply);

protected:
    Q_INVOKABLE void openMarketplace(const QString& marketplaceItemId = QString());
    Q_INVOKABLE void getMarketplaceItems(
        const QString& q = QString(),
        const QString& view = QString(),
        const QString& category = QString(),
        const QString& adminFilter = QString("published"),
        const QString& adminFilterCost = QString(),
        const QString& sort = QString(),
        bool isAscending = false,
        bool isFree = false,
        int page = 1,
        int perPage = 20);
    Q_INVOKABLE void getMarketplaceItem(const QString& marketplaceItemId);
    Q_INVOKABLE void marketplaceItemLike(const QString& marketplaceItemId, const bool like = true);
    Q_INVOKABLE void getMarketplaceCategories();

signals:
    void getMarketplaceItemsResult(QJsonObject result);
    void getMarketplaceItemResult(QJsonObject result);
    void getMarketplaceCategoriesResult(QJsonObject result);
    void marketplaceItemLikeResult(QJsonObject result);

private:
    void send(const QString& endpoint,
              const QString& success,
              const QString& fail,
              QNetworkAccessManager::Operation method,
              AccountManagerAuth::Type authType,
              const QUrlQuery& request = QUrlQuery());
    QJsonObject apiResponse(const QString& label, QNetworkReply* reply);
    QJsonObject failResponse(const QString& label, QNetworkReply* reply);
};

#endif // hifi_QmlMarketplace_h
