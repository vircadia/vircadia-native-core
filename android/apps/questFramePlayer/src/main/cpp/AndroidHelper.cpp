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

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>

AndroidHelper::AndroidHelper() {
}

AndroidHelper::~AndroidHelper() {
}

void AndroidHelper::notifyLoadComplete() {
    emit qtAppLoadComplete();
}

void AndroidHelper::notifyEnterForeground() {
    emit enterForeground();
}

void AndroidHelper::notifyEnterBackground() {
    emit enterBackground();
}

