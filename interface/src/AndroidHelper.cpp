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

AndroidHelper::AndroidHelper() :
_accountManager ()
{
    workerThread.start();
}

AndroidHelper::~AndroidHelper() {
    workerThread.quit();
    workerThread.wait();
}

QSharedPointer<AccountManager> AndroidHelper::getAccountManager() {
    if (!_accountManager) {
        _accountManager = QSharedPointer<AccountManager>(new AccountManager, &QObject::deleteLater);
        _accountManager->setIsAgent(true);
        _accountManager->setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL());
        _accountManager->moveToThread(&workerThread);
    }
    return _accountManager;
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

void AndroidHelper::notifyLoginComplete(bool success) {
    emit loginComplete(success);
}

void AndroidHelper::performHapticFeedback(const QString& feedbackConstant) {
    emit hapticFeedbackRequested(feedbackConstant);
}

