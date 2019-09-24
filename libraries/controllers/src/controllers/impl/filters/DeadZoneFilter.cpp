//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "DeadZoneFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

using namespace controller;
AxisValue DeadZoneFilter::apply(AxisValue value) const {
    float scale = ((value.value < 0.0f) ? -1.0f : 1.0f) / (1.0f - _min);
    float magnitude = std::abs(value.value);
    if (magnitude < _min) {
        return { 0.0f, value.timestamp };
    }
    return { (magnitude - _min) * scale, value.timestamp, value.valid };
}

bool DeadZoneFilter::parseParameters(const QJsonValue& parameters) {
    static const QString JSON_MIN = QStringLiteral("min");
    return parseSingleFloatParameter(parameters, JSON_MIN, _min);
}
