//
//  ClosureEventSender.cpp
//  interface/src/networking
//
//  Created by Stephen Birarda on 5/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QEventLoop>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QHttpMultiPart>
#include <QtNetwork/QNetworkReply>

#include <AccountManager.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <UserActivityLogger.h>
#include <UUID.h>

#include "ClosureEventSender.h"

QNetworkRequest createNetworkRequest() {

    QNetworkRequest request;

    QUrl requestURL = NetworkingConstants::METAVERSE_SERVER_URL;
    requestURL.setPath(USER_ACTIVITY_URL);

    request.setUrl(requestURL);

    auto accountManager = DependencyManager::get<AccountManager>();

    if (accountManager->hasValidAccessToken()) {
        request.setRawHeader(ACCESS_TOKEN_AUTHORIZATION_HEADER,
                             accountManager->getAccountInfo().getAccessToken().authorizationHeaderValue());
    }

    request.setRawHeader(METAVERSE_SESSION_ID_HEADER,
                         uuidStringWithoutCurlyBraces(accountManager->getSessionID()).toLocal8Bit());

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    return request;
}

QByteArray postDataForAction(QString action) {
    return QString("{\"action\": \"" + action + "\"}").toUtf8();
}

QNetworkReply* replyForAction(QString action) {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    return networkAccessManager.post(createNetworkRequest(), postDataForAction(action));
}

void ClosureEventSender::sendQuitStart() {

    QNetworkReply* reply = replyForAction("quit_start");

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
}

void ClosureEventSender::sendQuitFinish() {
    QNetworkReply* reply = replyForAction("quit_finish");

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
}
