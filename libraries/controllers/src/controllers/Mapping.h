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

#include <QtCore/QString>

#include "Endpoint.h"
#include "Filter.h"
#include "Route.h"

namespace Controllers {

    using ValueMap = std::map<Endpoint::Pointer, float>;

    class Mapping {
    public:
        // Map of source channels to route lists
        using Map = std::map<Endpoint::Pointer, Route::List>;
        using Pointer = std::shared_ptr<Mapping>;
        using List = std::list<Pointer>;

        Map _channelMappings;
        ValueMap _lastValues;

        void parse(const QString& json);
        QString serialize();
    };

}

#endif