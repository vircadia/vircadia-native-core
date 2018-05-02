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
}

AndroidHelper::~AndroidHelper() {
    workerThread.quit();
    workerThread.wait();
}

void AndroidHelper::init() {
    qDebug() << "[LOGIN] AndroidHelper::init";
    workerThread.start();
    _accountManager = DependencyManager::get<AccountManager>();
    _accountManager->moveToThread(&workerThread);
}

QSharedPointer<AccountManager> AndroidHelper::getAccountManager() {
    assert(_accountManager);
    return _accountManager;
}

void AndroidHelper::requestActivity(const QString &activityName, const bool backToScene) {
    emit androidActivityRequested(activityName, backToScene);
}

void AndroidHelper::notifyLoadComplete() {
    emit qtAppLoadComplete();
}

void AndroidHelper::notifyLoginComplete(bool success) {
    emit loginComplete(success);
}

void AndroidHelper::performHapticFeedback(const QString& feedbackConstant) {
    emit hapticFeedbackRequested(feedbackConstant);
}

void AndroidHelper::showLoginDialog() {
    emit androidActivityRequested("Login", true);
}
