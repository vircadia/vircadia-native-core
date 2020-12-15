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

#include <QEventLoop>
#include <QJsonDocument>
#include <QHttpMultiPart>
#include <QTimer>

#include <DependencyManager.h>

#include "AddressManager.h"
#include "NetworkLogging.h"

UserActivityLogger::UserActivityLogger() {
    _timer.start();
}

UserActivityLogger& UserActivityLogger::getInstance() {
    static UserActivityLogger sharedInstance;
    return sharedInstance;
}

void UserActivityLogger::disable(bool disable) {
    _disabled.set(disable);
}

void UserActivityLogger::crashMonitorDisable(bool disable) {
    _crashMonitorDisabled.set(disable);
}

void UserActivityLogger::logAction(QString action, QJsonObject details, JSONCallbackParameters params) {
//  qCDebug(networking).nospace() << ">>> UserActivityLogger::logAction(" << action << "," << QJsonDocument(details).toJson();
// This logs what the UserActivityLogger would normally send to centralized servers.
  return;
    if (_disabled.get()) {
        return;
    }

    auto accountManager = DependencyManager::get<AccountManager>();
    QHttpMultiPart* multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Adding the action name
    QHttpPart actionPart;
    actionPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"action_name\"");
    actionPart.setBody(QByteArray().append(action));
    multipart->append(actionPart);

    // Log the local-time that this event was logged
    QHttpPart elapsedPart;
    elapsedPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"elapsed_ms\"");
    elapsedPart.setBody(QString::number(_timer.elapsed()).toLocal8Bit());
    multipart->append(elapsedPart);

    // If there are action details, add them to the multipart
    if (!details.isEmpty()) {
        QHttpPart detailsPart;
        detailsPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                              " name=\"action_details\"");
        detailsPart.setBody(QJsonDocument(details).toJson(QJsonDocument::Compact));
        multipart->append(detailsPart);
    }

    // if no callbacks specified, call our owns
    if (params.isEmpty()) {
        params.callbackReceiver = this;
        params.errorCallbackMethod = "requestError";
    }

    accountManager->sendRequest(USER_ACTIVITY_URL,
                               AccountManagerAuth::Optional,
                               QNetworkAccessManager::PostOperation,
                               params, NULL, multipart);
}

void UserActivityLogger::requestError(QNetworkReply* errorReply) {
    qCDebug(networking) << errorReply->error() << "-" << errorReply->errorString();
}

void UserActivityLogger::launch(QString applicationVersion, bool previousSessionCrashed, int previousSessionRuntime) {
    const QString ACTION_NAME = "launch";
    QJsonObject actionDetails;
    QString VERSION_KEY = "version";
    QString CRASH_KEY = "previousSessionCrashed";
    QString RUNTIME_KEY = "previousSessionRuntime";
    actionDetails.insert(VERSION_KEY, applicationVersion);
    actionDetails.insert(CRASH_KEY, previousSessionCrashed);
    actionDetails.insert(RUNTIME_KEY, previousSessionRuntime);

    logAction(ACTION_NAME, actionDetails);
}

void UserActivityLogger::insufficientGLVersion(const QJsonObject& glData) {
    const QString ACTION_NAME = "insufficient_gl";
    QJsonObject actionDetails;
    QString GL_DATA = "glData";
    actionDetails.insert(GL_DATA, glData);

    logAction(ACTION_NAME, actionDetails);
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
    static QStringList DEVICE_BLACKLIST = {
        "Desktop",
        "NullDisplayPlugin",
        "3D TV - Side by Side Stereo",
        "3D TV - Interleaved",
        "Keyboard/Mouse"
    };

    if (DEVICE_BLACKLIST.contains(deviceName) || deviceName.isEmpty()) {
        return;
    }

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

void UserActivityLogger::wentTo(AddressManager::LookupTrigger lookupTrigger, QString destinationType, QString destinationName) {
    // Only accept these types of triggers. Other triggers are usually used internally in AddressManager.
    QString trigger;
    switch (lookupTrigger) {
        case AddressManager::UserInput:
            trigger = "UserInput";
            break;
        case AddressManager::Back:
            trigger = "Back";
            break;
        case AddressManager::Forward:
            trigger = "Forward";
            break;
        case AddressManager::StartupFromSettings:
            trigger = "StartupFromSettings";
            break;
        case AddressManager::Suggestions:
            trigger = "Suggestions";
            break;
        default:
            return;
    }


    const QString ACTION_NAME = "went_to";
    QJsonObject actionDetails;
    const QString TRIGGER_TYPE_KEY = "trigger";
    const QString DESTINATION_TYPE_KEY = "destination_type";
    const QString DESTINATION_NAME_KEY = "detination_name";

    actionDetails.insert(TRIGGER_TYPE_KEY, trigger);
    actionDetails.insert(DESTINATION_TYPE_KEY, destinationType);
    actionDetails.insert(DESTINATION_NAME_KEY, destinationName);

    logAction(ACTION_NAME, actionDetails);
}
