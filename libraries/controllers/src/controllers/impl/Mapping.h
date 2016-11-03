//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Mapping_h
#define hifi_Controllers_Mapping_h

#include <map>
#include <unordered_map>

#include <QtCore/QString>

#include "Endpoint.h"
#include "Filter.h"
#include "Route.h"

namespace controller {

    class Mapping {
    public:
        using Pointer = std::shared_ptr<Mapping>;
        using List = Route::List;

        Mapping(const QString& name) : name(name) {}

        List routes;
        QString name;
    };

}

#endif
