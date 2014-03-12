//
//  LocationManager.cpp
//  hifi
//
//  Created by Stojce Slavkovski on 2/7/14.
//
//

#include "LocationManager.h"

const QString GET_USER_ADDRESS = "/api/v1/users/%1/address";
const QString GET_PLACE_ADDRESS = "/api/v1/places/%1/address";
const QString GET_ADDRESSES = "/api/v1/addresses/%1/address";
const QString POST_PLACE_CREATE = "/api/v1/places/";

LocationManager& LocationManager::getInstance() {
    static LocationManager sharedInstance;
    return sharedInstance;
}

void LocationManager::namedLocationDataReceived(const QJsonObject& data) {
    if (data.isEmpty()) {
        return;
    }

    qDebug() << data;
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

        qDebug() << namedLocation->toJsonString();
        accountManager.authenticatedRequest(POST_PLACE_CREATE, QNetworkAccessManager::PostOperation,
                                            callbackParams, namedLocation->toJsonString().toUtf8());
    }
}


