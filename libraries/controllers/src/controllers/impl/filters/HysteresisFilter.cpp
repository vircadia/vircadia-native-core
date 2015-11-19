//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "HysteresisFilter.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

using namespace controller;

HysteresisFilter::HysteresisFilter(float min, float max) : _min(min), _max(max) {
    if (_min > _max) {
        std::swap(_min, _max);
    }
};


float HysteresisFilter::apply(float value) const {
    if (_signaled) {
        if (value <= _min) {
            _signaled = false;
        }
    } else {
        if (value >= _max) {
            _signaled = true;
        }
    }
    return _signaled ? 1.0f : 0.0f;
}

bool HysteresisFilter::parseParameters(const QJsonValue& parameters) {
    if (parameters.isArray()) {
        auto arrayParameters = parameters.toArray();
        if (arrayParameters.size() > 1) {
            _min = arrayParameters[0].toDouble();
        }
        if (arrayParameters.size() > 2) {
            _max = arrayParameters[1].toDouble();
        }
    } else if (parameters.isObject()) {
        static const QString JSON_MAX = QStringLiteral("max");
        static const QString JSON_MIN = QStringLiteral("min");

        auto objectParameters = parameters.toObject();
        if (objectParameters.contains(JSON_MIN)) {
            _min = objectParameters[JSON_MIN].toDouble();
        }
        if (objectParameters.contains(JSON_MAX)) {
            _max = objectParameters[JSON_MAX].toDouble();
        }
    } else {
        return false;
    }

    if (_min > _max) {
        std::swap(_min, _max);
    }
    return true;
}
