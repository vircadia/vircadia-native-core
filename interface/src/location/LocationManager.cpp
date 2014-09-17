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

#include <qhttpmultipart.h>
#include <qjsonobject.h>

#include <AccountManager.h>

#include "Application.h"
#include "ui/Snapshot.h"

#include "LocationManager.h"

const QString POST_LOCATION_CREATE = "/api/v1/locations/";

LocationManager& LocationManager::getInstance() {
    static LocationManager sharedInstance;
    return sharedInstance;
}

const QString UNKNOWN_ERROR_MESSAGE = "Unknown error creating named location. Please try again!";

const QString LOCATION_OBJECT_KEY = "location";
const QString LOCATION_ID_KEY = "id";

void LocationManager::namedLocationDataReceived(const QJsonObject& rootObject) {

    if (rootObject.contains("status") && rootObject["status"].toString() == "success") {
        emit creationCompleted(QString());
        
        // successfuly created a location - grab the ID from the response and create a snapshot to upload
        QString locationIDString = rootObject[LOCATION_OBJECT_KEY].toObject()[LOCATION_ID_KEY].toString();
        updateSnapshotForExistingLocation(locationIDString);
    } else {
        emit creationCompleted(UNKNOWN_ERROR_MESSAGE);
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

void LocationManager::locationImageUpdateSuccess(const QJsonObject& rootObject) {
    qDebug() << "Successfuly updated a location image.";
}

void LocationManager::updateSnapshotForExistingLocation(const QString& locationID) {
    // first create a snapshot and save it
    Application* application = Application::getInstance();
    
    QTemporaryFile* tempImageFile = Snapshot::saveTempSnapshot(application->getGLWidget(), application->getAvatar());
    
    if (tempImageFile && tempImageFile->open()) {
        AccountManager& accountManager = AccountManager::getInstance();
        
        // setup a multipart that is in the AccountManager thread - we need this so it can be cleaned up after the QNetworkReply
        QHttpMultiPart* imageFileMultiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        imageFileMultiPart->moveToThread(accountManager.thread());
        
        // parent the temp file to the QHttpMultipart after moving it to account manager thread
        tempImageFile->moveToThread(accountManager.thread());
        tempImageFile->setParent(imageFileMultiPart);
        
        qDebug() << "Uploading a snapshot from" << QFileInfo(*tempImageFile).absoluteFilePath()
            << "as location image for" << locationID;
        
        const QString LOCATION_IMAGE_NAME = "location[image]";
        
        QHttpPart imagePart;
        imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                            QVariant("form-data; name=\"" + LOCATION_IMAGE_NAME +  "\";"
                                     " filename=\"" + QFileInfo(tempImageFile->fileName()).fileName().toUtf8() +  "\""));
        imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
        imagePart.setBodyDevice(tempImageFile);
        
        imageFileMultiPart->append(imagePart);
        
        const QString LOCATION_IMAGE_PUT_PATH = "api/v1/locations/%1/image";
        
        JSONCallbackParameters imageCallbackParams;
        imageCallbackParams.jsonCallbackReceiver = this;
        imageCallbackParams.jsonCallbackMethod = "locationImageUpdateSuccess";
        
        // make an authenticated request via account manager to upload the image
        // don't do anything with error or success for now
        AccountManager::getInstance().authenticatedRequest(LOCATION_IMAGE_PUT_PATH.arg(locationID),
                                                           QNetworkAccessManager::PutOperation,
                                                           JSONCallbackParameters(), QByteArray(), imageFileMultiPart);
    } else {
        qDebug() << "Couldn't open snapshot file to upload as location image. No location image will be stored.";
        return;
    }
}


