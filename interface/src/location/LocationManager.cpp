//
//  LocationManager.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/7/14.
//
//

#include "LocationManager.h"
#include "Application.h"

const QString GET_USER_ADDRESS = "/api/v1/users/%1/address";
const QString GET_PLACE_ADDRESS = "/api/v1/places/%1/address";
const QString POST_PLACE_CREATE = "/api/v1/places/";


LocationManager::LocationManager() : _userData(), _placeData() {

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
        NamedLocation* location = new NamedLocation(data["data"].toObject());
        emit creationCompleted(LocationManager::Created, location);
    } else {
        emit creationCompleted(LocationManager::AlreadyExists, NULL);
    }
}

void LocationManager::errorDataReceived(QNetworkReply::NetworkError error, const QString& message) {
    emit creationCompleted(LocationManager::SystemError, NULL);
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

    if (destination.startsWith("@")) {
        // remove '@' and go to user
        goToUser(destination.remove(0, 1));
        return;
    }

    if (destination.startsWith("#")) {
        // remove '#' and go to named place
        goToPlace(destination.remove(0, 1));
        return;
    }

    // go to coordinate destination or to Username
    if (!goToDestination(destination)) {
        // reset data on local variables
        _userData = QJsonObject();
        _placeData = QJsonObject();

        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "goToUserFromResponse";
        AccountManager::getInstance().authenticatedRequest(GET_USER_ADDRESS.arg(destination),
                                                           QNetworkAccessManager::GetOperation,
                                                           callbackParams);

        callbackParams.jsonCallbackMethod = "goToLocationFromResponse";
        AccountManager::getInstance().authenticatedRequest(GET_PLACE_ADDRESS.arg(destination),
                                                           QNetworkAccessManager::GetOperation,
                                                           callbackParams);
    }
}

void LocationManager::goToUserFromResponse(const QJsonObject& jsonObject) {
    _userData = jsonObject;
    checkForMultipleDestinations();
}

void LocationManager::goToLocationFromResponse(const QJsonObject& jsonObject) {
    _placeData = jsonObject;
    checkForMultipleDestinations();
}

void LocationManager::checkForMultipleDestinations() {
    if (!_userData.isEmpty() && !_placeData.isEmpty()) {
        if (_userData.contains("status") && _userData["status"].toString() == "success" &&
            _placeData.contains("status") && _placeData["status"].toString() == "success") {
            emit multipleDestinationsFound(_userData, _placeData);
            return;
        }

        if (_userData.contains("status") && _userData["status"].toString() == "success") {
            Application::getInstance()->getAvatar()->goToLocationFromResponse(_userData);
            return;
        }

        if (_placeData.contains("status") && _placeData["status"].toString() == "success") {
            Application::getInstance()->getAvatar()->goToLocationFromResponse(_placeData);
            return;
        }

        emit locationChanged();
    }
}

void LocationManager::goToUser(QString userName) {

    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = Application::getInstance()->getAvatar();
    callbackParams.jsonCallbackMethod = "goToLocationFromResponse";

    AccountManager::getInstance().authenticatedRequest(GET_USER_ADDRESS.arg(userName),
                                                       QNetworkAccessManager::GetOperation,
                                                       callbackParams);
}

void LocationManager::goToPlace(QString placeName) {
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = Application::getInstance()->getAvatar();
    callbackParams.jsonCallbackMethod = "goToLocationFromResponse";

    AccountManager::getInstance().authenticatedRequest(GET_PLACE_ADDRESS.arg(placeName),
                                                       QNetworkAccessManager::GetOperation,
                                                       callbackParams);
}

void LocationManager::goToOrientation(QString orientation) {
    if (orientation.isEmpty()) {
        return;
    }

    QStringList orientationItems = orientation.split(QRegExp("_|,"), QString::SkipEmptyParts);

    const int NUMBER_OF_ORIENTATION_ITEMS = 4;
    const int W_ITEM = 0;
    const int X_ITEM = 1;
    const int Y_ITEM = 2;
    const int Z_ITEM = 3;

    if (orientationItems.size() == NUMBER_OF_ORIENTATION_ITEMS) {

        double w = replaceLastOccurrence('-', '.', orientationItems[W_ITEM].trimmed()).toDouble();
        double x = replaceLastOccurrence('-', '.', orientationItems[X_ITEM].trimmed()).toDouble();
        double y = replaceLastOccurrence('-', '.', orientationItems[Y_ITEM].trimmed()).toDouble();
        double z = replaceLastOccurrence('-', '.', orientationItems[Z_ITEM].trimmed()).toDouble();

        glm::quat newAvatarOrientation(w, x, y, z);

        MyAvatar* myAvatar = Application::getInstance()->getAvatar();
        glm::quat avatarOrientation = myAvatar->getOrientation();
        if (newAvatarOrientation != avatarOrientation) {
            myAvatar->setOrientation(newAvatarOrientation);
        }
    }
}

bool LocationManager::goToDestination(QString destination) {

    QStringList coordinateItems = destination.split(QRegExp("_|,"), QString::SkipEmptyParts);

    const int NUMBER_OF_COORDINATE_ITEMS = 3;
    const int X_ITEM = 0;
    const int Y_ITEM = 1;
    const int Z_ITEM = 2;
    if (coordinateItems.size() == NUMBER_OF_COORDINATE_ITEMS) {

        double x = replaceLastOccurrence('-', '.', coordinateItems[X_ITEM].trimmed()).toDouble();
        double y = replaceLastOccurrence('-', '.', coordinateItems[Y_ITEM].trimmed()).toDouble();
        double z = replaceLastOccurrence('-', '.', coordinateItems[Z_ITEM].trimmed()).toDouble();

        glm::vec3 newAvatarPos(x, y, z);

        MyAvatar* myAvatar = Application::getInstance()->getAvatar();
        glm::vec3 avatarPos = myAvatar->getPosition();
        if (newAvatarPos != avatarPos) {
            // send a node kill request, indicating to other clients that they should play the "disappeared" effect
            MyAvatar::sendKillAvatar();

            qDebug("Going To Location: %f, %f, %f...", x, y, z);
            myAvatar->setPosition(newAvatarPos);
        }

        return true;
    }

    // no coordinates were parsed
    return false;
}

QString LocationManager::replaceLastOccurrence(QChar search, QChar replace, QString string) {
    int lastIndex;
    lastIndex = string.lastIndexOf(search);
    if (lastIndex > 0) {
        lastIndex = string.lastIndexOf(search);
        string.replace(lastIndex, 1, replace);
    }

    return string;
}

