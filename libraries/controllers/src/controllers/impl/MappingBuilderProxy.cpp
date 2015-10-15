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

#include "RouteBuilderProxy.h"
#include "../ScriptingInterface.h"
#include "../Logging.h"

namespace controller {

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

QObject* MappingBuilderProxy::join(const QJSValue& source1, const QJSValue& source2) {
    auto source1Endpoint = _parent.endpointFor(source1);
    auto source2Endpoint = _parent.endpointFor(source2);
    return from(_parent.compositeEndpointFor(source1Endpoint, source2Endpoint));
}

QObject* MappingBuilderProxy::enable(bool enable) {
    _parent.enableMapping(_mapping->_name, enable);
    return this;
}

}
