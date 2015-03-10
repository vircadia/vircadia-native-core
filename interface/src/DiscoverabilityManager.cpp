//
//  DiscoverabilityManager.cpp
//  interface/src
//
//  Created by Stephen Birarda on 2015-03-09.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QJsonDocument>

#include <AccountManager.h>
#include <AddressManager.h>
#include <DomainHandler.h>
#include <NodeList.h>
#include <UUID.h>

#include "DiscoverabilityManager.h"

const Discoverability::Mode DEFAULT_DISCOVERABILITY_MODE = Discoverability::None;

DiscoverabilityManager::DiscoverabilityManager() :
    _mode("discoverabilityMode", DEFAULT_DISCOVERABILITY_MODE)
{
    
}

const QString API_USER_LOCATION_PATH = "/api/v1/user/location";

void DiscoverabilityManager::updateLocation() {
    if (_mode.get() != Discoverability::None) {
        AccountManager& accountManager = AccountManager::getInstance();
        auto addressManager = DependencyManager::get<AddressManager>();
        DomainHandler& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
        
        if (accountManager.isLoggedIn() && domainHandler.isConnected()
            && (!addressManager->getRootPlaceID().isNull() || !domainHandler.getUUID().isNull())) {
            
            // construct a QJsonObject given the user's current address information
            QJsonObject rootObject;
            
            QJsonObject locationObject;
            
            QString pathString = addressManager->currentPath();
            
            const QString LOCATION_KEY_IN_ROOT = "location";
            
            const QString PATH_KEY_IN_LOCATION = "path";
            locationObject.insert(PATH_KEY_IN_LOCATION, pathString);
            
            if (!addressManager->getRootPlaceID().isNull()) {
                const QString PLACE_ID_KEY_IN_LOCATION = "place_id";
                locationObject.insert(PLACE_ID_KEY_IN_LOCATION,
                                      uuidStringWithoutCurlyBraces(addressManager->getRootPlaceID()));
                
            } else {
                const QString DOMAIN_ID_KEY_IN_LOCATION = "domain_id";
                locationObject.insert(DOMAIN_ID_KEY_IN_LOCATION,
                                      uuidStringWithoutCurlyBraces(domainHandler.getUUID()));
            }
            
            rootObject.insert(LOCATION_KEY_IN_ROOT, locationObject);
            
            accountManager.authenticatedRequest(API_USER_LOCATION_PATH, QNetworkAccessManager::PutOperation,
                                                JSONCallbackParameters(), QJsonDocument(rootObject).toJson());
        }
    }
}

void DiscoverabilityManager::removeLocation() {
    AccountManager& accountManager = AccountManager::getInstance();
    accountManager.authenticatedRequest(API_USER_LOCATION_PATH, QNetworkAccessManager::DeleteOperation);
}

void DiscoverabilityManager::setDiscoverabilityMode(Discoverability::Mode discoverabilityMode) {
    if (_mode.get() != discoverabilityMode) {
        
        // update the setting to the new value
        _mode.set(discoverabilityMode);
        
        if (_mode.get() == Discoverability::None) {
            // if we just got set to no discoverability, make sure that we delete our location in DB
            removeLocation();
        }
    }
}