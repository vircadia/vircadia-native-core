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

using namespace controller;

void RouteBuilderProxy::toQml(const QJSValue& destination) {
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
    auto sourceEndpoint = _route->source;
    _route->destination = destination;
    _mapping->routes.push_back(_route);
    deleteLater();
}

QObject* RouteBuilderProxy::filterQml(const QJSValue& expression) {
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
    _route->filters.push_back(filter);
}

