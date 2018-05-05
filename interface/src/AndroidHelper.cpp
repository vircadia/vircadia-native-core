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

void AndroidHelper::requestActivity(const QString &activityName) {
    emit androidActivityRequested(activityName);
}

void AndroidHelper::goBackFromAndroidActivity() {
    emit backFromAndroidActivity();
}