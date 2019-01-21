//
//  QmlMarketplace.cpp
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


#include "QmlMarketplace.h"
#include "CommerceLogging.h"
#include "Application.h"
#include "DependencyManager.h"
#include <Application.h>
#include <UserActivityLogger.h>
#include <ScriptEngines.h>
#include <ui/TabletScriptingInterface.h>
#include "scripting/HMDScriptingInterface.h"

#define ApiHandler(NAME) void QmlMarketplace::NAME##Success(QNetworkReply* reply) { emit NAME##Result(apiResponse(#NAME, reply)); }
#define FailHandler(NAME) void QmlMarketplace::NAME##Failure(QNetworkReply* reply) { emit NAME##Result(failResponse(#NAME, reply)); }
#define Handler(NAME) ApiHandler(NAME) FailHandler(NAME)
Handler(getMarketplaceItems)
Handler(getMarketplaceItem)
Handler(marketplaceItemLike)
Handler(getMarketplaceCategories)

QmlMarketplace::QmlMarketplace() {
}

void QmlMarketplace::openMarketplace(const QString& marketplaceItemId) {
    auto tablet = dynamic_cast<TabletProxy*>(
        DependencyManager::get<TabletScriptingInterface>()->getTablet("com.highfidelity.interface.tablet.system"));
    tablet->loadQMLSource("hifi/commerce/marketplace/Marketplace.qml");
    DependencyManager::get<HMDScriptingInterface>()->openTablet();
    if (!marketplaceItemId.isEmpty()) {
        tablet->sendToQml(QVariantMap({ { "method", "marketplace_openItem" }, { "itemId", marketplaceItemId } }));
    }
}

void QmlMarketplace::getMarketplaceItems(
    const QString& q,
    const QString& view,
    const QString& category,
    const QString& adminFilter,
    const QString& adminFilterCost,
    const QString& sort,
    const bool isFree,
    const int& page,
    const int& perPage) {

    QString endpoint = "items";
    QJsonObject request;
    request["q"] = q;
    request["view"] = view;
    request["category"] = category;
    request["adminFilter"] = adminFilter;
    request["adminFilterCost"] = adminFilterCost;
    request["sort"] = sort;
    request["isFree"] = isFree;
    request["page"] = page;
    request["perPage"] = perPage;
    send(endpoint, "getMarketplaceItemsSuccess", "getMarketplaceItemsFailure", QNetworkAccessManager::GetOperation, AccountManagerAuth::Optional, request);
}

void QmlMarketplace::getMarketplaceItem(const QString& marketplaceItemId) {
    QString endpoint = QString("items/") + marketplaceItemId;
    QJsonObject request;
    send(endpoint, "getMarketplaceItemSuccess", "getMarketplaceItemFailure", QNetworkAccessManager::GetOperation, AccountManagerAuth::Optional, request);
}

void QmlMarketplace::marketplaceItemLike(const QString& marketplaceItemId, const bool like) {
    QString endpoint = QString("items/") + marketplaceItemId + "/like";
    QJsonObject request;
    send(endpoint, "marketplaceItemLikeSuccess", "marketplaceItemLikeFailure", like ? QNetworkAccessManager::PutOperation : QNetworkAccessManager::DeleteOperation, AccountManagerAuth::Required, request);
}

void QmlMarketplace::getMarketplaceCategories() {
    QString endpoint = "categories";
    QJsonObject request;
    send(endpoint, "getMarketplaceCategoriesSuccess", "getMarketplaceCategoriesFailure", QNetworkAccessManager::GetOperation, AccountManagerAuth::None, request);
}


void QmlMarketplace::send(const QString& endpoint, const QString& success, const QString& fail, QNetworkAccessManager::Operation method, AccountManagerAuth::Type authType, QJsonObject request) {
    auto accountManager = DependencyManager::get<AccountManager>();
    const QString URL = "/api/v1/marketplace/";
    JSONCallbackParameters callbackParams(this, success, fail);
#if defined(DEV_BUILD)  // Don't expose user's personal data in the wild. But during development this can be handy.
    qCInfo(commerce) << "Sending" << QJsonDocument(request).toJson(QJsonDocument::Compact);
#endif
    accountManager->sendRequest(URL + endpoint,
        authType,
        method,
        callbackParams,
        QJsonDocument(request).toJson());
}

QJsonObject QmlMarketplace::apiResponse(const QString& label, QNetworkReply* reply) {
    QByteArray response = reply->readAll();
    QJsonObject data = QJsonDocument::fromJson(response).object();
#if defined(DEV_BUILD)  // Don't expose user's personal data in the wild. But during development this can be handy.
    qInfo(commerce) << label << "response" << QJsonDocument(data).toJson(QJsonDocument::Compact);
#endif
    return data;
}

// Non-200 responses are not json:
QJsonObject QmlMarketplace::failResponse(const QString& label, QNetworkReply* reply) {
    QString response = reply->readAll();
    qWarning(commerce) << "FAILED" << label << response;

    // tempResult will be NULL if the response isn't valid JSON.
    QJsonDocument tempResult = QJsonDocument::fromJson(response.toLocal8Bit());
    if (tempResult.isNull()) {
        QJsonObject result
        {
            { "status", "fail" },
            { "message", response }
        };
        return result;
    }
    else {
        return tempResult.object();
    }
}