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

#include <QJSonObject>
#include <QJSonArray>

#include "SharedUtil.h"

using namespace controller;


const QString JSON_FILTER_TYPE = QStringLiteral("type");
const QString JSON_FILTER_PARAMS = QStringLiteral("params");

Filter::Factory Filter::_factory;

Filter::Pointer Filter::parse(const QJsonObject& json) {
    // The filter is an object, now let s check for type and potential arguments
    Filter::Pointer filter;
    auto filterType = json[JSON_FILTER_TYPE];
    if (filterType.isString()) {
        filter.reset(Filter::getFactory().create(filterType.toString().toStdString()));
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

ScaleFilter::FactoryEntryBuilder ScaleFilter::_factoryEntryBuilder;

bool ScaleFilter::parseParameters(const QJsonArray& parameters) {
    if (parameters.size() > 1) {
        _scale = parameters[0].toDouble();
    }
    return true;
}

InvertFilter::FactoryEntryBuilder InvertFilter::_factoryEntryBuilder;


ClampFilter::FactoryEntryBuilder ClampFilter::_factoryEntryBuilder;

bool ClampFilter::parseParameters(const QJsonArray& parameters) {
    if (parameters.size() > 1) {
        _min = parameters[0].toDouble();
    }
    if (parameters.size() > 2) {
        _max = parameters[1].toDouble();
    }
    return true;
}

DeadZoneFilter::FactoryEntryBuilder DeadZoneFilter::_factoryEntryBuilder;

float DeadZoneFilter::apply(float value) const {
    float scale = 1.0f / (1.0f - _min);
    if (abs(value) < _min) {
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

PulseFilter::FactoryEntryBuilder PulseFilter::_factoryEntryBuilder;


float PulseFilter::apply(float value) const {
    float result = 0.0;

    if (0.0 != value) {
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

ConstrainToIntegerFilter::FactoryEntryBuilder ConstrainToIntegerFilter::_factoryEntryBuilder;

ConstrainToPositiveIntegerFilter::FactoryEntryBuilder ConstrainToPositiveIntegerFilter::_factoryEntryBuilder;

