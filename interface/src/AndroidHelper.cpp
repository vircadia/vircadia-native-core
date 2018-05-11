//
//  AndroidHelper.cpp
//  interface/src
//
//  Created by Gabriel Calero & Cristian Duarte on 3/30/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "AndroidHelper.h"
#include <QDebug>
#include <AccountManager.h>

AndroidHelper::AndroidHelper() {
}

AndroidHelper::~AndroidHelper() {
    workerThread.quit();
    workerThread.wait();
}

void AndroidHelper::init() {
    workerThread.start();
    _accountManager = QSharedPointer<AccountManager>(new AccountManager, &QObject::deleteLater);
    _accountManager->setIsAgent(true);
    _accountManager->setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL());
    _accountManager->setSessionID(DependencyManager::get<AccountManager>()->getSessionID());
    connect(_accountManager.data(), &AccountManager::loginComplete, [](const QUrl& authURL) {
            DependencyManager::get<AccountManager>()->setAccountInfo(AndroidHelper::instance().getAccountManager()->getAccountInfo());
            DependencyManager::get<AccountManager>()->setAuthURL(authURL);
    });

    connect(_accountManager.data(), &AccountManager::logoutComplete, [] () {
            DependencyManager::get<AccountManager>()->logout();
    });
    _accountManager->moveToThread(&workerThread);
}

void AndroidHelper::requestActivity(const QString &activityName) {
    emit androidActivityRequested(activityName);
}

void AndroidHelper::notifyLoadComplete() {
    emit qtAppLoadComplete();
}

void AndroidHelper::goBackFromAndroidActivity() {
    emit backFromAndroidActivity();
}

