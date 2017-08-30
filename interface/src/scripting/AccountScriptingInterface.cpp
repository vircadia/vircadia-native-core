//
//  AccountScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Stojce Slavkovski on 6/07/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AccountManager.h"

#include "AccountScriptingInterface.h"

AccountScriptingInterface* AccountScriptingInterface::getInstance() {
    static AccountScriptingInterface sharedInstance;
    auto accountManager = DependencyManager::get<AccountManager>();
    QObject::connect(accountManager.data(), &AccountManager::profileChanged,
                     &sharedInstance, &AccountScriptingInterface::usernameChanged);
    QObject::connect(accountManager.data(), &AccountManager::usernameChanged,
                     &sharedInstance, &AccountScriptingInterface::onUsernameChanged);
    return &sharedInstance;
}

bool AccountScriptingInterface::isLoggedIn() {
    auto accountManager = DependencyManager::get<AccountManager>();
    return accountManager->isLoggedIn();
}

bool AccountScriptingInterface::checkAndSignalForAccessToken() {
    auto accountManager = DependencyManager::get<AccountManager>();
    return accountManager->checkAndSignalForAccessToken();
}

void AccountScriptingInterface::logOut() {
    auto accountManager = DependencyManager::get<AccountManager>();
    return accountManager->logout();
}

AccountScriptingInterface::AccountScriptingInterface(QObject *parent): QObject(parent) {
    m_loggedIn = isLoggedIn();
    emit loggedInChanged(m_loggedIn);
}

void AccountScriptingInterface::onUsernameChanged(QString username) {
    m_loggedIn = (username != QString());
    emit loggedInChanged(m_loggedIn);
}

QString AccountScriptingInterface::getUsername() {
    auto accountManager = DependencyManager::get<AccountManager>();
    if (accountManager->isLoggedIn()) {
        return accountManager->getAccountInfo().getUsername();
    } else {
        return "Unknown user";
    }
}
