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
#include "Application.h"

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application*>(QCoreApplication::instance()))

AndroidHelper::AndroidHelper() {
}

AndroidHelper::~AndroidHelper() {
}

void AndroidHelper::requestActivity(const QString &activityName, const bool backToScene, QList<QString> args) {
    emit androidActivityRequested(activityName, backToScene, args);
}

void AndroidHelper::notifyLoadComplete() {
    emit qtAppLoadComplete();
}

void AndroidHelper::notifyEnterForeground() {
    emit enterForeground();
}

void AndroidHelper::notifyBeforeEnterBackground() {
    emit beforeEnterBackground();
}

void AndroidHelper::notifyEnterBackground() {
    emit enterBackground();
}

void AndroidHelper::performHapticFeedback(int duration) {
    emit hapticFeedbackRequested(duration);
}

void AndroidHelper::showLoginDialog() {
    emit androidActivityRequested("Login", true);
}

void AndroidHelper::processURL(const QString &url) {
    if (qApp->canAcceptURL(url)) {
        qApp->acceptURL(url);
    }
}
