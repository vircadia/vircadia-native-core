//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Filter.h"

#include <QtCore/QObject>
#include <QtScript/QScriptValue>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include "SharedUtil.h"

using namespace controller;

Filter::Factory Filter::_factory;

REGISTER_FILTER_CLASS_INSTANCE(InvertFilter, "invert")
REGISTER_FILTER_CLASS_INSTANCE(ConstrainToIntegerFilter, "constrainToInteger")
REGISTER_FILTER_CLASS_INSTANCE(ConstrainToPositiveIntegerFilter, "constrainToPositiveInteger")
REGISTER_FILTER_CLASS_INSTANCE(ScaleFilter, "scale")
REGISTER_FILTER_CLASS_INSTANCE(ClampFilter, "clamp")
REGISTER_FILTER_CLASS_INSTANCE(DeadZoneFilter, "deadZone")
REGISTER_FILTER_CLASS_INSTANCE(PulseFilter, "pulse")


const QString JSON_FILTER_TYPE = QStringLiteral("type");
const QString JSON_FILTER_PARAMS = QStringLiteral("params");


Filter::Pointer Filter::parse(const QJsonObject& json) {
    // The filter is an object, now let s check for type and potential arguments
    Filter::Pointer filter;
    auto filterType = json[JSON_FILTER_TYPE];
    if (filterType.isString()) {
        filter = Filter::getFactory().create(filterType.toString());
        if (filter) {
            // Filter is defined, need to read the parameters and validate
            auto parameters = json[JSON_FILTER_PARAMS];
            if (parameters.isArray()) {
                if (filter->parseParameters(parameters.toArray())) {
                }
            }

            return filter;
        }
    }
    return Filter::Pointer();
}


bool ScaleFilter::parseParameters(const QJsonArray& parameters) {
    if (parameters.size() > 1) {
        _scale = parameters[0].toDouble();
    }
    return true;
}


bool ClampFilter::parseParameters(const QJsonArray& parameters) {
    if (parameters.size() > 1) {
        _min = parameters[0].toDouble();
    }
    if (parameters.size() > 2) {
        _max = parameters[1].toDouble();
    }
    return true;
}


float DeadZoneFilter::apply(float value) const {
    float scale = 1.0f / (1.0f - _min);
    if (std::abs(value) < _min) {
        return 0.0f;
    }
    return (value - _min) * scale;
}

bool DeadZoneFilter::parseParameters(const QJsonArray& parameters) {
    if (parameters.size() > 1) {
        _min = parameters[0].toDouble();
    }
    return true;
}


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

bool PulseFilter::parseParameters(const QJsonArray& parameters) {
    if (parameters.size() > 1) {
        _interval = parameters[0].toDouble();
    }
    return true;
}


