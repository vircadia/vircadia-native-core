//
//  Created by Brad Hefta-Gaub 2017/04/11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RotateFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <StreamUtils.h>

using namespace controller;

bool RotateFilter::parseParameters(const QJsonValue& parameters) {
    static const QString JSON_ROTATION = QStringLiteral("rotation");
    return parseQuatParameter(parameters, JSON_ROTATION, _rotation);
}
