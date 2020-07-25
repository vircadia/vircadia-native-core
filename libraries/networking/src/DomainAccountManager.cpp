//
//  DomainAccountManager.cpp
//  libraries/networking/src
//
//  Created by David Rowe on 23 Jul 2020.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DomainAccountManager.h"

#include <QTimer>


DomainAccountManager::DomainAccountManager() {

}

bool DomainAccountManager::hasValidAccessToken() {

    // #######: TODO

    return false;
}

bool DomainAccountManager::checkAndSignalForAccessToken() {
    bool hasToken = hasValidAccessToken();

    if (!hasToken) {
        // Emit a signal so somebody can call back to us and request an access token given a user name and password.

        // Dialog can be hidden immediately after showing if we've just teleported to the domain, unless the signal is delayed.
        QTimer::singleShot(500, this, [this] { emit this->authRequired(); });
    }

    return hasToken;
}
