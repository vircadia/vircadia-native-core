//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MappingBuilderProxy.h"

#include <QtCore/QHash>
#include <QtCore/QDebug>

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>


#include "RouteBuilderProxy.h"
#include "../ScriptingInterface.h"
#include "../Logging.h"

using namespace controller;

QObject* MappingBuilderProxy::from(const QJSValue& source) {
    qCDebug(controllers) << "Creating new Route builder proxy from " << source.toString();
    auto sourceEndpoint = _parent.endpointFor(source);
    return from(sourceEndpoint);
}

QObject* MappingBuilderProxy::from(const QScriptValue& source) {
    qCDebug(controllers) << "Creating new Route builder proxy from " << source.toString();
    auto sourceEndpoint = _parent.endpointFor(source);
    return from(sourceEndpoint);
}

QObject* MappingBuilderProxy::from(const Endpoint::Pointer& source) {
    auto route = Route::Pointer(new Route());
    route->_source = source;
    return new RouteBuilderProxy(_parent, _mapping, route);
}

QObject* MappingBuilderProxy::makeAxis(const QJSValue& source1, const QJSValue& source2) {
    auto source1Endpoint = _parent.endpointFor(source1);
    auto source2Endpoint = _parent.endpointFor(source2);
    return from(_parent.compositeEndpointFor(source1Endpoint, source2Endpoint));
}

const QString JSON_NAME = QStringLiteral("name");
const QString JSON_CHANNELS = QStringLiteral("channels");
const QString JSON_CHANNEL_FROM = QStringLiteral("from");
const QString JSON_CHANNEL_TO = QStringLiteral("to");
const QString JSON_CHANNEL_FILTERS = QStringLiteral("filters");


void MappingBuilderProxy::parse(const QJsonObject& json) {
    _mapping->_name = json[JSON_NAME].toString();

    _mapping->_channelMappings.clear();
    const auto& jsonChannels = json[JSON_CHANNELS].toArray();
    for (const auto& channelIt : jsonChannels) {
        parseRoute(channelIt);
    }
}

void MappingBuilderProxy::parseRoute(const QJsonValue& json) {
    if (json.isObject()) {
        const auto& jsonChannel = json.toObject();

        auto newRoute = from(jsonChannel[JSON_CHANNEL_FROM]);
        if (newRoute) {
            auto route = dynamic_cast<RouteBuilderProxy*>(newRoute);
            route->filters(jsonChannel[JSON_CHANNEL_FILTERS]);
            route->to(jsonChannel[JSON_CHANNEL_TO]);
        }
    }
}

QObject* MappingBuilderProxy::from(const QJsonValue& json) {
    if (json.isString()) {
        return from(_parent.endpointFor(_parent.inputFor(json.toString())));
    } else if (json.isObject()) {
        // Endpoint is defined as an object, we expect a js function then
        return nullptr;
    }
    return nullptr;
}

QObject* MappingBuilderProxy::enable(bool enable) {
    _parent.enableMapping(_mapping->_name, enable);
    return this;
}


