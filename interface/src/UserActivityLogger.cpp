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
#include <QJsonDocument>

static const QString USER_ACTIVITY_URL = "/api/v1/user_activities";

UserActivityLogger& UserActivityLogger::getInstance() {
    static UserActivityLogger sharedInstance;
    return sharedInstance;
}

UserActivityLogger::UserActivityLogger() {
}

void UserActivityLogger::logAction(QString action, QJsonObject details) {
    AccountManager& accountManager = AccountManager::getInstance();
    QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    if (action != "login") {
        QHttpPart actionPart;
        actionPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"action_name\"");
        actionPart.setBody(QByteArray().append(action));
        multipart->append(actionPart);
        
        
        if (!details.isEmpty()) {
            QHttpPart detailsPart;
            detailsPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                                  " name=\"action_details\"");
            detailsPart.setBody(QJsonDocument(details).toJson(QJsonDocument::Compact));
            multipart->append(detailsPart);
        }
    }
    qDebug() << "Loging activity " << action;
    
    JSONCallbackParameters params;
    params.jsonCallbackReceiver = this;
    params.jsonCallbackMethod = "requestFinished";
    params.errorCallbackReceiver = this;
    params.errorCallbackMethod = "requestError";
    
    accountManager.authenticatedRequest(USER_ACTIVITY_URL,
                                        QNetworkAccessManager::PostOperation,
                                        params,
                                        NULL,
                                        multipart);
}

void UserActivityLogger::requestFinished(const QJsonObject& object) {
    qDebug() << object;
}

void UserActivityLogger::requestError(QNetworkReply::NetworkError error,const QString& string) {
    qDebug() << error << ": " << string;
}

void UserActivityLogger::login() {
    const QString ACTION_NAME = "login";
    QJsonObject actionDetails;
    
    QString OS_KEY = "OS";
    QString VERSION_KEY = "Version";
#ifdef Q_OS_MAC
    actionDetails.insert(OS_KEY, QJsonValue(QString("Mac")));
    actionDetails.insert(VERSION_KEY, QJsonValue(QSysInfo::macVersion()));
#elif Q_OS_LINUX
    actionDetails.insert(OS_KEY, QJsonValue(QString("Linux")));
#elif Q_OS_WIN
    actionDetails.insert(OS_KEY, QJsonValue(QString("Windows")));
    actionDetails.insert(VERSION_KEY, QJsonValue(QSysInfo::windowsVersion()));
#elif Q_OS_UNIX
    actionDetails.insert(OS_KEY, QJsonValue(QString("Unknown UNIX")));
#else
    actionDetails.insert(OS_KEY, QJsonValue(QString("Unknown system")));
#endif
    
    logAction(ACTION_NAME, actionDetails);
}

void UserActivityLogger::logout() {
    const QString ACTION_NAME = "";
    logAction(ACTION_NAME);
}

void UserActivityLogger::changedModel() {
    const QString ACTION_NAME = "changed_model";
    QJsonObject actionDetails;
    
    
    logAction(ACTION_NAME, actionDetails);
}

void UserActivityLogger::changedDomain() {
    const QString ACTION_NAME = "changed_domain";
    QJsonObject actionDetails;
    
    
    logAction(ACTION_NAME, actionDetails);
}

