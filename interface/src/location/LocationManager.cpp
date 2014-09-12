//
//  LocationManager.cpp
//  interface/src/location
//
//  Created by Stojce Slavkovski on 2/7/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qjsonobject.h>

#include <AccountManager.h>

#include "LocationManager.h"

const QString POST_LOCATION_CREATE = "/api/v1/locations/";

LocationManager& LocationManager::getInstance() {
    static LocationManager sharedInstance;
    return sharedInstance;
}

const QString UNKNOWN_ERROR_MESSAGE = "Unknown error creating named location. Please try again!";

void LocationManager::namedLocationDataReceived(const QJsonObject& data) {
    if (data.isEmpty()) {
        return;
    }

    if (data.contains("status") && data["status"].toString() == "success") {
        emit creationCompleted(QString());
    } else {
        emit  creationCompleted(UNKNOWN_ERROR_MESSAGE);
    }
}

void LocationManager::createNamedLocation(NamedLocation* namedLocation) {
    AccountManager& accountManager = AccountManager::getInstance();
    if (accountManager.isLoggedIn()) {
        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "namedLocationDataReceived";
        callbackParams.errorCallbackReceiver = this;
        callbackParams.errorCallbackMethod = "errorDataReceived";

        accountManager.authenticatedRequest(POST_LOCATION_CREATE, QNetworkAccessManager::PostOperation,
                                            callbackParams, namedLocation->toJsonString().toUtf8());
    }
}

void LocationManager::errorDataReceived(QNetworkReply& errorReply) {
    
    if (errorReply.header(QNetworkRequest::ContentTypeHeader).toString().startsWith("application/json")) {
        // we have some JSON error data we can parse for our error message
        QJsonDocument responseJson = QJsonDocument::fromJson(errorReply.readAll());
        
        QJsonObject dataObject = responseJson.object()["data"].toObject();
        
        qDebug() << dataObject;
        
        QString errorString = "There was a problem creating that location.\n";
        
        // construct the error string from the returned attribute errors
        foreach(const QString& key, dataObject.keys()) {
            errorString += "\n\u2022 " + key + " - ";
            
            QJsonValue keyedErrorValue = dataObject[key];
           
            if (keyedErrorValue.isArray()) {
                foreach(const QJsonValue& attributeErrorValue, keyedErrorValue.toArray()) {
                    errorString += attributeErrorValue.toString() + ", ";
                }
                
                // remove the trailing comma at end of error list
                errorString.remove(errorString.length() - 2, 2);
            } else if (keyedErrorValue.isString()) {
                errorString += keyedErrorValue.toString();
            }           
        }
        
        // emit our creationCompleted signal with the error
        emit creationCompleted(errorString);
        
    } else {
        creationCompleted(UNKNOWN_ERROR_MESSAGE);
    }
}
