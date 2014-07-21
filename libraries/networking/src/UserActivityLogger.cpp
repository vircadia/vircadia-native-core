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

#include <QEventLoop>
#include <QJsonDocument>
#include <QHttpMultiPart>
#include <QTimer>

#include "UserActivityLogger.h"

static const QString USER_ACTIVITY_URL = "/api/v1/user_activities";

UserActivityLogger& UserActivityLogger::getInstance() {
    static UserActivityLogger sharedInstance;
    return sharedInstance;
}

UserActivityLogger::UserActivityLogger() : _disabled(false) {
}

void UserActivityLogger::disable(bool disable) {
    _disabled = disable;
}

void UserActivityLogger::logAction(QString action, QJsonObject details, JSONCallbackParameters params) {
    if (_disabled) {
        return;
    }
    
    AccountManager& accountManager = AccountManager::getInstance();
    QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // Adding the action name
    QHttpPart actionPart;
    actionPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"action_name\"");
    actionPart.setBody(QByteArray().append(action));
    multipart->append(actionPart);
    
    // If there are action details, add them to the multipart
    if (!details.isEmpty()) {
        QHttpPart detailsPart;
        detailsPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                              " name=\"action_details\"");
        detailsPart.setBody(QJsonDocument(details).toJson(QJsonDocument::Compact));
        multipart->append(detailsPart);
    }
    qDebug() << "Logging activity" << action;
    
    // if no callbacks specified, call our owns
    if (params.isEmpty()) {
        params.jsonCallbackReceiver = this;
        params.jsonCallbackMethod = "requestFinished";
        params.errorCallbackReceiver = this;
        params.errorCallbackMethod = "requestError";
    }
    
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

void UserActivityLogger::launch(QString applicationVersion) {
    const QString ACTION_NAME = "launch";
    QJsonObject actionDetails;
    QString VERSION_KEY = "version";
    actionDetails.insert(VERSION_KEY, applicationVersion);
    
    logAction(ACTION_NAME, actionDetails);
}

void UserActivityLogger::close(int delayTime) {
    const QString ACTION_NAME = "close";
    
    // In order to get the end of the session, we need to give the account manager enough time to send the packet.
    QEventLoop loop;
    QTimer timer;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    // Now we can log it
    logAction(ACTION_NAME, QJsonObject());
    timer.start(delayTime);
    loop.exec();
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

void UserActivityLogger::wentTo(QString destinationType, QString destinationName) {
    const QString ACTION_NAME = "went_to";
    QJsonObject actionDetails;
    const QString DESTINATION_TYPE_KEY = "destination_type";
    const QString DESTINATION_NAME_KEY = "detination_name";
    
    actionDetails.insert(DESTINATION_TYPE_KEY, destinationType);
    actionDetails.insert(DESTINATION_NAME_KEY, destinationName);
    
    logAction(ACTION_NAME, actionDetails);
}
