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

AccountScriptingInterface::AccountScriptingInterface() {
    AccountManager& accountManager = AccountManager::getInstance();
    connect(&accountManager, &AccountManager::balanceChanged, this,
            &AccountScriptingInterface::updateBalance);
}

AccountScriptingInterface* AccountScriptingInterface::getInstance() {
    static AccountScriptingInterface sharedInstance;
    return &sharedInstance;
}

qint64 AccountScriptingInterface::getBalance() {
    AccountManager& accountManager = AccountManager::getInstance();
    return accountManager.getAccountInfo().getBalance();
}

bool AccountScriptingInterface::isLoggedIn() {
    AccountManager& accountManager = AccountManager::getInstance();
    return accountManager.isLoggedIn();
}

void AccountScriptingInterface::updateBalance(qint16 newBalance) {
    emit balanceChanged(newBalance);
}
