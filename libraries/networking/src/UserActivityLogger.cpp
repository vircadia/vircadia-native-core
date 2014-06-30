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

#include "AccountManager.h"

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

void UserActivityLogger::changedDisplayName(QString displayName) {
    const QString ACTION_NAME = "changed_display_name";
    QJsonObject actionDetails;
    const QString DISPLAY_NAME = "display_name";
    
    actionDetails.insert(DISPLAY_NAME, displayName);
    
    logAction(ACTION_NAME, actionDetails);
}

void UserActivityLogger::changedModel(QString typeOfModel, QString modelURL) {
    const QString ACTION_NAME = "changed_model";
    QJsonObject actionDetails;
    const QString TYPE_OF_MODEL = "type_of_model";
    const QString MODEL_URL = "model_url";
    
    actionDetails.insert(TYPE_OF_MODEL, typeOfModel);
    actionDetails.insert(MODEL_URL, modelURL);
    
    logAction(ACTION_NAME, actionDetails);
}

void UserActivityLogger::changedDomain(QString domainURL) {
    const QString ACTION_NAME = "changed_domain";
    QJsonObject actionDetails;
    const QString DOMAIN_URL = "domain_url";
    
    actionDetails.insert(DOMAIN_URL, domainURL);
    
    logAction(ACTION_NAME, actionDetails);
}

void UserActivityLogger::connectedDevice(QString typeOfDevice, QString deviceName) {
    const QString ACTION_NAME = "connected_device";
    QJsonObject actionDetails;
    const QString TYPE_OF_DEVICE = "type_of_device";
    const QString DEVICE_NAME = "device_name";
    
    actionDetails.insert(TYPE_OF_DEVICE, typeOfDevice);
    actionDetails.insert(DEVICE_NAME, deviceName);
    
    logAction(ACTION_NAME, actionDetails);

}

void UserActivityLogger::loadedScript(QString scriptName) {
    const QString ACTION_NAME = "loaded_script";
    QJsonObject actionDetails;
    const QString SCRIPT_NAME = "script_name";
    
    actionDetails.insert(SCRIPT_NAME, scriptName);
    
    logAction(ACTION_NAME, actionDetails);

}
