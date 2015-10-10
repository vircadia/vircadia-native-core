//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Route_h
#define hifi_Controllers_Route_h

#include "Endpoint.h"
#include "Filter.h"

class QJsonObject;

namespace Controllers {

    /*
    * encapsulates a source, destination and filters to apply
    */
    class Route {
    public:
        Endpoint::Pointer _source;
        Endpoint::Pointer _destination;
        Filter::List _filters;

        using Pointer = std::shared_ptr<Route>;
        using List = std::list<Pointer>;

        void parse(const QJsonObject& json);
    };
}

#endif
