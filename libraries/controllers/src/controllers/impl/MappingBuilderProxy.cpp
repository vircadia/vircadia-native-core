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

namespace Controllers {

QObject* MappingBuilderProxy::from(const QString& source) {
    qDebug() << "Creating new Route builder proxy from " << source;
    auto route = Route::Pointer(new Route());
    route->_source = endpointFor(source);
    return new RouteBuilderProxy(this, route);
}

Endpoint::Pointer MappingBuilderProxy::endpointFor(const QString& endpoint) {
    static QHash<QString, Endpoint::Pointer> ENDPOINTS;
    if (!ENDPOINTS.contains(endpoint)) {
        ENDPOINTS[endpoint] = std::make_shared<Endpoint>();
    }
    return ENDPOINTS[endpoint];
}

}
