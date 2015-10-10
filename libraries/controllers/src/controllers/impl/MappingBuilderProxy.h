//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_Controllers_Impl_MappingBuilderProxy_h
#define hifi_Controllers_Impl_MappingBuilderProxy_h

#include <QtCore/QObject>
#include <QtCore/QString>

#include "../Mapping.h"

namespace Controllers {

class MappingBuilderProxy : public QObject {
    Q_OBJECT
public:
    MappingBuilderProxy(Mapping::Pointer mapping)
        : _mapping(mapping) { }

    Q_INVOKABLE QObject* from(const QString& fromEndpoint);

protected:
    friend class RouteBuilderProxy;
    Endpoint::Pointer endpointFor(const QString& endpoint);
    Mapping::Pointer _mapping;
};

}

#endif
