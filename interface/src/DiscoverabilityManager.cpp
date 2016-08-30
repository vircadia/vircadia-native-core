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
#include <steamworks-wrapper/SteamClient.h>
#include <UserActivityLogger.h>
#include <UUID.h>

#include "DiscoverabilityManager.h"
#include "Menu.h"

const Discoverability::Mode DEFAULT_DISCOVERABILITY_MODE = Discoverability::Friends;

DiscoverabilityManager::DiscoverabilityManager() :
    _mode("discoverabilityMode", DEFAULT_DISCOVERABILITY_MODE)
{
    qRegisterMetaType<Discoverability::Mode>("Discoverability::Mode");
}

const QString API_USER_LOCATION_PATH = "/api/v1/user/location";
const QString API_USER_HEARTBEAT_PATH = "/api/v1/user/heartbeat";

const QString SESSION_ID_KEY = "session_id";

void DiscoverabilityManager::updateLocation() {
    auto accountManager = DependencyManager::get<AccountManager>();
    auto addressManager = DependencyManager::get<AddressManager>();
    auto& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();


    if (_mode.get() != Discoverability::None && accountManager->isLoggedIn()) {
        // construct a QJsonObject given the user's current address information
        QJsonObject rootObject;

        QJsonObject locationObject;

        QString pathString = addressManager->currentPath();

        const QString PATH_KEY_IN_LOCATION = "path";
        locationObject.insert(PATH_KEY_IN_LOCATION, pathString);

        const QString CONNECTED_KEY_IN_LOCATION = "connected";
        locationObject.insert(CONNECTED_KEY_IN_LOCATION, domainHandler.isConnected());

        if (!addressManager->getRootPlaceID().isNull()) {
            const QString PLACE_ID_KEY_IN_LOCATION = "place_id";
            locationObject.insert(PLACE_ID_KEY_IN_LOCATION,
                                  uuidStringWithoutCurlyBraces(addressManager->getRootPlaceID()));
        }

        if (!domainHandler.getUUID().isNull()) {
            const QString DOMAIN_ID_KEY_IN_LOCATION = "domain_id";
            locationObject.insert(DOMAIN_ID_KEY_IN_LOCATION,
                                  uuidStringWithoutCurlyBraces(domainHandler.getUUID()));
        }

        // in case the place/domain isn't in the database, we send the network address and port
        auto& domainSockAddr = domainHandler.getSockAddr();
        const QString NETWORK_ADRESS_KEY_IN_LOCATION = "network_address";
        locationObject.insert(NETWORK_ADRESS_KEY_IN_LOCATION, domainSockAddr.getAddress().toString());

        const QString NETWORK_ADDRESS_PORT_IN_LOCATION = "network_port";
        locationObject.insert(NETWORK_ADDRESS_PORT_IN_LOCATION, domainSockAddr.getPort());

        const QString FRIENDS_ONLY_KEY_IN_LOCATION = "friends_only";
        locationObject.insert(FRIENDS_ONLY_KEY_IN_LOCATION, (_mode.get() == Discoverability::Friends));

        JSONCallbackParameters callbackParameters;
        callbackParameters.jsonCallbackReceiver = this;
        callbackParameters.jsonCallbackMethod = "handleHeartbeatResponse";

        // figure out if we'll send a fresh location or just a simple heartbeat
        auto apiPath = API_USER_HEARTBEAT_PATH;

        if (locationObject != _lastLocationObject) {
            // we have a changed location, send it now
            _lastLocationObject = locationObject;

            const QString LOCATION_KEY_IN_ROOT = "location";
            rootObject.insert(LOCATION_KEY_IN_ROOT, locationObject);

            apiPath = API_USER_LOCATION_PATH;
        }

        accountManager->sendRequest(apiPath, AccountManagerAuth::Required,
                                   QNetworkAccessManager::PutOperation,
                                   callbackParameters, QJsonDocument(rootObject).toJson());

    } else if (UserActivityLogger::getInstance().isEnabled()) {
        // we still send a heartbeat to the metaverse server for stats collection

        JSONCallbackParameters callbackParameters;
        callbackParameters.jsonCallbackReceiver = this;
        callbackParameters.jsonCallbackMethod = "handleHeartbeatResponse";

        accountManager->sendRequest(API_USER_HEARTBEAT_PATH, AccountManagerAuth::Optional,
                                   QNetworkAccessManager::PutOperation, callbackParameters);
    }

    // Update Steam
    SteamClient::updateLocation(domainHandler.getHostname(), addressManager->currentFacingShareableAddress());
}

void DiscoverabilityManager::handleHeartbeatResponse(QNetworkReply& requestReply) {
    auto dataObject = AccountManager::dataObjectFromResponse(requestReply);

    if (!dataObject.isEmpty()) {
        auto sessionID = dataObject[SESSION_ID_KEY].toString();

        // give that session ID to the account manager
        auto accountManager = DependencyManager::get<AccountManager>();
        accountManager->setSessionID(sessionID);
    }
}

void DiscoverabilityManager::removeLocation() {
    auto accountManager = DependencyManager::get<AccountManager>();
    accountManager->sendRequest(API_USER_LOCATION_PATH, AccountManagerAuth::Required, QNetworkAccessManager::DeleteOperation);
}

void DiscoverabilityManager::setDiscoverabilityMode(Discoverability::Mode discoverabilityMode) {
    if (static_cast<Discoverability::Mode>(_mode.get()) != discoverabilityMode) {
        
        // update the setting to the new value
        _mode.set(static_cast<int>(discoverabilityMode));
        
        if (static_cast<int>(_mode.get()) == Discoverability::None) {
            // if we just got set to no discoverability, make sure that we delete our location in DB
            removeLocation();
        } else {
            // we have a discoverability mode that says we should send a location, do that right away
            updateLocation();
        }

        emit discoverabilityModeChanged(discoverabilityMode);
    }
}

void DiscoverabilityManager::setVisibility() {
    Menu* menu = Menu::getInstance();

    if (menu->isOptionChecked(MenuOption::VisibleToEveryone)) {
        this->setDiscoverabilityMode(Discoverability::All);
    } else if (menu->isOptionChecked(MenuOption::VisibleToFriends)) {
        this->setDiscoverabilityMode(Discoverability::Friends);
    } else if (menu->isOptionChecked(MenuOption::VisibleToNoOne)) {
        this->setDiscoverabilityMode(Discoverability::None);
    } else {
        qDebug() << "ERROR DiscoverabilityManager::setVisibility() called with unrecognized value.";
    }
}

void DiscoverabilityManager::visibilityChanged(Discoverability::Mode discoverabilityMode) {
    Menu* menu = Menu::getInstance();

    if (discoverabilityMode == Discoverability::All) {
        menu->setIsOptionChecked(MenuOption::VisibleToEveryone, true);
    } else if (discoverabilityMode == Discoverability::Friends) {
        menu->setIsOptionChecked(MenuOption::VisibleToFriends, true);
    } else if (discoverabilityMode == Discoverability::None) {
        menu->setIsOptionChecked(MenuOption::VisibleToNoOne, true);
    } else {
        qDebug() << "ERROR DiscoverabilityManager::visibilityChanged() called with unrecognized value.";
    }
}
