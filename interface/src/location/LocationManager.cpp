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

#include <AccountManager.h>

#include "LocationManager.h"

const QString POST_LOCATION_CREATE = "/api/v1/locations/";

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
