//
//  CloseEventSender.cpp
//  interface/src/networking
//
//  Created by Stephen Birarda on 5/31/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDateTime>
#include <QtCore/QEventLoop>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkReply>

#include <ThreadHelpers.h>
#include <AccountManager.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <NetworkLogging.h>
#include <UserActivityLogger.h>
#include <UUID.h>

#include "CloseEventSender.h"

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

    request.setPriority(QNetworkRequest::HighPriority);

    return request;
}

QByteArray postDataForAction(QString action) {
    return QString("{\"action_name\": \"" + action + "\"}").toUtf8();
}

QNetworkReply* replyForAction(QString action) {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    return networkAccessManager.post(createNetworkRequest(), postDataForAction(action));
}

void CloseEventSender::sendQuitEventAsync() {
    if (UserActivityLogger::getInstance().isEnabled()) {
        QNetworkReply* reply = replyForAction("quit");
        connect(reply, &QNetworkReply::finished, this, &CloseEventSender::handleQuitEventFinished);
        _quitEventStartTimestamp = QDateTime::currentMSecsSinceEpoch();
    } else {
        _hasFinishedQuitEvent = true;
    }
}

void CloseEventSender::handleQuitEventFinished() {
    _hasFinishedQuitEvent = true;

    auto reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() == QNetworkReply::NoError) {
        qCDebug(networking) << "Quit event sent successfully";
    } else {
        qCDebug(networking) << "Failed to send quit event -" << reply->errorString();
    }

    reply->deleteLater();
}

bool CloseEventSender::hasTimedOutQuitEvent() {
    const int CLOSURE_EVENT_TIMEOUT_MS = 5000;
    return _quitEventStartTimestamp != 0
        && QDateTime::currentMSecsSinceEpoch() - _quitEventStartTimestamp > CLOSURE_EVENT_TIMEOUT_MS;
}

void CloseEventSender::startThread() {
    moveToNewNamedThread(this, "CloseEvent Logger Thread", [this] { 
        sendQuitEventAsync(); 
    });
}
