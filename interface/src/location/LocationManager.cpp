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

#include <QMessageBox>

#include "Application.h"
#include "LocationManager.h"
#include <UserActivityLogger.h>

const QString GET_USER_ADDRESS = "/api/v1/users/%1/address";
const QString GET_PLACE_ADDRESS = "/api/v1/places/%1";
const QString GET_ADDRESSES = "/api/v1/addresses/%1";
const QString POST_PLACE_CREATE = "/api/v1/places/";


LocationManager::LocationManager() {

};

LocationManager& LocationManager::getInstance() {
    static LocationManager sharedInstance;
    return sharedInstance;
}

void LocationManager::namedLocationDataReceived(const QJsonObject& data) {
    if (data.isEmpty()) {
        return;
    }

    if (data.contains("status") && data["status"].toString() == "success") {
        emit creationCompleted(LocationManager::Created);
    } else {
        emit creationCompleted(LocationManager::AlreadyExists);
    }
}

void LocationManager::errorDataReceived(QNetworkReply::NetworkError error, const QString& message) {
    emit creationCompleted(LocationManager::SystemError);
}

void LocationManager::createNamedLocation(NamedLocation* namedLocation) {
    AccountManager& accountManager = AccountManager::getInstance();
    if (accountManager.isLoggedIn()) {
        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "namedLocationDataReceived";
        callbackParams.errorCallbackReceiver = this;
        callbackParams.errorCallbackMethod = "errorDataReceived";

        accountManager.authenticatedRequest(POST_PLACE_CREATE, QNetworkAccessManager::PostOperation,
                                            callbackParams, namedLocation->toJsonString().toUtf8());
    }
}

void LocationManager::goTo(QString destination) {
    const QString USER_DESTINATION_TYPE = "user";
    const QString PLACE_DESTINATION_TYPE = "place";
    const QString OTHER_DESTINATION_TYPE = "coordinate_or_username";
    
    if (destination.startsWith("@")) {
        // remove '@' and go to user
        QString destinationUser = destination.remove(0, 1);
        UserActivityLogger::getInstance().wentTo(USER_DESTINATION_TYPE, destinationUser);
        goToUser(destinationUser);
        return;
    }

    if (destination.startsWith("#")) {
        // remove '#' and go to named place
        QString destinationPlace = destination.remove(0, 1);
        UserActivityLogger::getInstance().wentTo(PLACE_DESTINATION_TYPE, destinationPlace);
        goToPlace(destinationPlace);
        return;
    }

    // go to coordinate destination or to Username
    if (!goToDestination(destination)) {
        destination = QString(QUrl::toPercentEncoding(destination));
        UserActivityLogger::getInstance().wentTo(OTHER_DESTINATION_TYPE, destination);
        
        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "goToAddressFromResponse";
        callbackParams.errorCallbackReceiver = this;
        callbackParams.errorCallbackMethod = "handleAddressLookupError";
        
        AccountManager::getInstance().authenticatedRequest(GET_ADDRESSES.arg(destination),
                                                           QNetworkAccessManager::GetOperation,
                                                           callbackParams);
    }
}

void LocationManager::goToAddressFromResponse(const QJsonObject& responseData) {
    QJsonValue status = responseData["status"];
    
    const QJsonObject& data = responseData["data"].toObject();
    const QJsonValue& userObject = data["user"];
    const QJsonValue& placeObject = data["place"];
    
    if (!placeObject.isUndefined() && !userObject.isUndefined()) {
        emit multipleDestinationsFound(userObject.toObject(), placeObject.toObject());
    } else if (placeObject.isUndefined()) {
        Application::getInstance()->getAvatar()->goToLocationFromAddress(userObject.toObject()["address"].toObject());
    } else {
        Application::getInstance()->getAvatar()->goToLocationFromAddress(placeObject.toObject()["address"].toObject());
    }
}

void LocationManager::goToUser(QString userName) {
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = Application::getInstance()->getAvatar();
    callbackParams.jsonCallbackMethod = "goToLocationFromResponse";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "handleAddressLookupError";

    userName = QString(QUrl::toPercentEncoding(userName));
    AccountManager::getInstance().authenticatedRequest(GET_USER_ADDRESS.arg(userName),
                                                       QNetworkAccessManager::GetOperation,
                                                       callbackParams);
}

void LocationManager::goToPlace(QString placeName) {
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = Application::getInstance()->getAvatar();
    callbackParams.jsonCallbackMethod = "goToLocationFromResponse";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "handleAddressLookupError";

    placeName = QString(QUrl::toPercentEncoding(placeName));
    AccountManager::getInstance().authenticatedRequest(GET_PLACE_ADDRESS.arg(placeName),
                                                       QNetworkAccessManager::GetOperation,
                                                       callbackParams);
}

void LocationManager::goToOrientation(QString orientation) {
    if (orientation.isEmpty()) {
        return;
    }

    QStringList orientationItems = orientation.remove(' ').split(QRegExp("_|,"), QString::SkipEmptyParts);

    const int NUMBER_OF_ORIENTATION_ITEMS = 4;
    const int W_ITEM = 0;
    const int X_ITEM = 1;
    const int Y_ITEM = 2;
    const int Z_ITEM = 3;

    if (orientationItems.size() == NUMBER_OF_ORIENTATION_ITEMS) {

        // replace last occurrence of '_' with decimal point
        replaceLastOccurrence('-', '.', orientationItems[W_ITEM]);
        replaceLastOccurrence('-', '.', orientationItems[X_ITEM]);
        replaceLastOccurrence('-', '.', orientationItems[Y_ITEM]);
        replaceLastOccurrence('-', '.', orientationItems[Z_ITEM]);

        double w = orientationItems[W_ITEM].toDouble();
        double x = orientationItems[X_ITEM].toDouble();
        double y = orientationItems[Y_ITEM].toDouble();
        double z = orientationItems[Z_ITEM].toDouble();

        glm::quat newAvatarOrientation(w, x, y, z);

        MyAvatar* myAvatar = Application::getInstance()->getAvatar();
        glm::quat avatarOrientation = myAvatar->getOrientation();
        if (newAvatarOrientation != avatarOrientation) {
            myAvatar->setOrientation(newAvatarOrientation);
            emit myAvatar->transformChanged();
        }
    }
}

bool LocationManager::goToDestination(QString destination) {

    QStringList coordinateItems = destination.remove(' ').split(QRegExp("_|,"), QString::SkipEmptyParts);

    const int NUMBER_OF_COORDINATE_ITEMS = 3;
    const int X_ITEM = 0;
    const int Y_ITEM = 1;
    const int Z_ITEM = 2;
    if (coordinateItems.size() == NUMBER_OF_COORDINATE_ITEMS) {

        // replace last occurrence of '_' with decimal point
        replaceLastOccurrence('-', '.', coordinateItems[X_ITEM]);
        replaceLastOccurrence('-', '.', coordinateItems[Y_ITEM]);
        replaceLastOccurrence('-', '.', coordinateItems[Z_ITEM]);

        double x = coordinateItems[X_ITEM].toDouble();
        double y = coordinateItems[Y_ITEM].toDouble();
        double z = coordinateItems[Z_ITEM].toDouble();

        glm::vec3 newAvatarPos(x, y, z);

        MyAvatar* myAvatar = Application::getInstance()->getAvatar();
        glm::vec3 avatarPos = myAvatar->getPosition();
        if (newAvatarPos != avatarPos) {
            // send a node kill request, indicating to other clients that they should play the "disappeared" effect
            MyAvatar::sendKillAvatar();

            qDebug("Going To Location: %f, %f, %f...", x, y, z);
            myAvatar->setPosition(newAvatarPos);
            emit myAvatar->transformChanged();
        }

        return true;
    }

    // no coordinates were parsed
    return false;
}

void LocationManager::handleAddressLookupError(QNetworkReply::NetworkError networkError,
                                               const QString& errorString) {
    QString messageBoxString;
    
    if (networkError == QNetworkReply::ContentNotFoundError) {
        messageBoxString = "That address could not be found.";
    } else {
        messageBoxString = errorString;
    }
    
    QMessageBox::warning(Application::getInstance()->getWindow(), "", messageBoxString);
}

void LocationManager::replaceLastOccurrence(const QChar search, const QChar replace, QString& string) {
    int lastIndex;
    lastIndex = string.lastIndexOf(search);
    if (lastIndex > 0) {
        lastIndex = string.lastIndexOf(search);
        string.replace(lastIndex, 1, replace);
    }
}
