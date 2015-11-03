//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PulseFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

using namespace controller;



float PulseFilter::apply(float value) const {
    float result = 0.0f;

    if (0.0f != value) {
        float now = secTimestampNow();
        float delta = now - _lastEmitTime;
        if (delta >= _interval) {
            _lastEmitTime = now;
            result = value;
        }
    }

    return result;
}

bool PulseFilter::parseParameters(const QJsonValue& parameters) {
    static const QString JSON_MIN = QStringLiteral("interval");
    return parseSingleFloatParameter(parameters, JSON_MIN, _interval);
}

