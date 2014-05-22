//
//  UserActivityLogger.cpp
//
//
//  Created by Clement on 5/21/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UserActivityLogger.h"

#include <AccountManager.h>

#include <QHttpMultiPart>

static const QString USER_ACTIVITY_URL = "/api/v1/user_activities";

UserActivityLogger& UserActivityLogger::getInstance() {
    static UserActivityLogger sharedInstance;
    return sharedInstance;
}

UserActivityLogger::UserActivityLogger() {
    
}

void UserActivityLogger::logAction(QString action, QString details) {
    AccountManager& accountManager = AccountManager::getInstance();
    QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    QHttpPart actionPart;
    actionPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"action\"");
    actionPart.setBody(QByteArray().append(action));
    multipart->append(actionPart);
    
    
    if (!details.isEmpty()) {
        QHttpPart detailsPart;
        detailsPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                              " name=\"details\"");
        detailsPart.setBody(QByteArray().append(details));
        multipart->append(detailsPart);
    }
    
    accountManager.authenticatedRequest(USER_ACTIVITY_URL,
                                        QNetworkAccessManager::PostOperation,
                                        JSONCallbackParameters(),
                                        NULL,
                                        multipart);
}

void UserActivityLogger::login() {
    const QString ACTION_NAME = "login";
    logAction(ACTION_NAME);
}

void UserActivityLogger::logout() {
    const QString ACTION_NAME = "logout";
    logAction(ACTION_NAME);
}