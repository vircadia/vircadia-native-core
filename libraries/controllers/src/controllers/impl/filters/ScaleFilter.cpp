//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScaleFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

using namespace controller;

bool ScaleFilter::parseParameters(const QJsonValue& parameters) {
    static const QString JSON_SCALE = QStringLiteral("scale");
    return parseSingleFloatParameter(parameters, JSON_SCALE, _scale);
}
