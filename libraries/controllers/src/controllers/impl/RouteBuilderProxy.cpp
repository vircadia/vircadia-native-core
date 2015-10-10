//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "RouteBuilderProxy.h"

#include <QtCore/QDebug>

#include <GLMHelpers.h>

#include "MappingBuilderProxy.h"

namespace Controllers {

void RouteBuilderProxy::to(const QString& destination) {
    qDebug() << "Completed route: " << destination;
    auto sourceEndpoint = _route->_source;
    auto& mapping = _parent->_mapping;
    mapping->_channelMappings[sourceEndpoint].push_back(_route);
    deleteLater();
}

QObject* RouteBuilderProxy::clamp(float min, float max) {
    addFilter([=](float value) {
        return glm::clamp(value, min, max);
    });
    return this;
}

QObject* RouteBuilderProxy::scale(float multiplier) {
    addFilter([=](float value) {
        return value * multiplier;
    });
    return this;
}

QObject* RouteBuilderProxy::invert() {
    return scale(-1.0f);
}

QObject* RouteBuilderProxy::deadZone(float min) {
    assert(min < 1.0f);
    float scale = 1.0f / (1.0f - min);
    addFilter([=](float value) {
        if (value < min) {
            return 0.0f;
        }
        return (value - min) * scale;
    });

    return this;
}

QObject* RouteBuilderProxy::constrainToInteger() {
    addFilter([=](float value) {
        return glm::sign(value);
    });
    return this;
}

QObject* RouteBuilderProxy::constrainToPositiveInteger() {
    addFilter([=](float value) {
        return (value <= 0.0f) ? 0.0f : 1.0f;
    });
    return this;
}

void RouteBuilderProxy::addFilter(Filter::Lambda lambda) {
    Filter::Pointer filterPointer = std::make_shared < LambdaFilter > (lambda);
    addFilter(filterPointer);
}

void RouteBuilderProxy::addFilter(Filter::Pointer filter) {
    _route->_filters.push_back(filter);
}

}
