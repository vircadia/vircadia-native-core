//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RouteBuilderProxy.h"

#include <QtCore/QDebug>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <GLMHelpers.h>

#include "MappingBuilderProxy.h"
#include "../ScriptingInterface.h"
#include "../Logging.h"

namespace controller {

void RouteBuilderProxy::to(int destinationInput) {
    qCDebug(controllers) << "Completing route " << destinationInput;
    auto destinationEndpoint = _parent.endpointFor(UserInputMapper::Input(destinationInput));
    return to(destinationEndpoint);
}

void RouteBuilderProxy::to(const QJSValue& destination) {
    qCDebug(controllers) << "Completing route " << destination.toString();
    auto destinationEndpoint = _parent.endpointFor(destination);
    return to(destinationEndpoint);
}

void RouteBuilderProxy::to(const QScriptValue& destination) {
    qCDebug(controllers) << "Completing route " << destination.toString();
    auto destinationEndpoint = _parent.endpointFor(destination);
    return to(destinationEndpoint);
}

void RouteBuilderProxy::to(const Endpoint::Pointer& destination) {
    auto sourceEndpoint = _route->_source;
    _route->_destination = destination;
    _mapping->_channelMappings[sourceEndpoint].push_back(_route);
    deleteLater();
}

QObject* RouteBuilderProxy::filter(const QJSValue& expression) {
    if (expression.isCallable()) {
        addFilter([=](float value) {
            QJSValue originalExpression = expression;
            QJSValueList params({ QJSValue(value) });
            auto result = originalExpression.call(params);
            return (float)(result.toNumber());
        });
    }
    return this;
}

QObject* RouteBuilderProxy::filter(const QScriptValue& expression) {
    return this;
}


QObject* RouteBuilderProxy::clamp(float min, float max) {
    addFilter(Filter::Pointer(new ClampFilter(min, max)));
    return this;
}

QObject* RouteBuilderProxy::scale(float multiplier) {
    addFilter(Filter::Pointer(new ScaleFilter(multiplier)));
    return this;
}

QObject* RouteBuilderProxy::invert() {
    addFilter(Filter::Pointer(new InvertFilter()));
    return this;
}

QObject* RouteBuilderProxy::deadZone(float min) {
    addFilter(Filter::Pointer(new DeadZoneFilter(min)));
    return this;
}

QObject* RouteBuilderProxy::constrainToInteger() {
    addFilter(Filter::Pointer(new ConstrainToIntegerFilter()));
    return this;
}

QObject* RouteBuilderProxy::constrainToPositiveInteger() {
    addFilter(Filter::Pointer(new ConstrainToPositiveIntegerFilter()));
    return this;
}


QObject* RouteBuilderProxy::pulse(float interval) {
    addFilter(Filter::Pointer(new PulseFilter(interval)));
    return this;
}

void RouteBuilderProxy::addFilter(Filter::Lambda lambda) {
    Filter::Pointer filterPointer = std::make_shared < LambdaFilter > (lambda);
    addFilter(filterPointer);
}

void RouteBuilderProxy::addFilter(Filter::Pointer filter) {
    _route->_filters.push_back(filter);
}


QObject* RouteBuilderProxy::filters(const QJsonValue& json) {
    // We expect an array of objects to define the filters
    if (json.isArray()) {
        const auto& jsonFilters = json.toArray();
        for (auto jsonFilter : jsonFilters) {
            if (jsonFilter.isObject()) {
                // The filter is an object, now let s check for type and potential arguments
                Filter::Pointer filter = Filter::parse(jsonFilter.toObject());
                if (filter) {
                    addFilter(filter);
                }
            }
        }
    }

    return this;
}

void RouteBuilderProxy::to(const QJsonValue& json) {
    if (json.isString()) {

        return to(_parent.endpointFor(_parent.inputFor(json.toString())));
    } else if (json.isObject()) {
        // Endpoint is defined as an object, we expect a js function then
        //return to((Endpoint*) nullptr);
    }

}

}
