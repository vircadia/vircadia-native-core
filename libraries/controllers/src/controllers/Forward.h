//
//  Created by Bradley Austin Davis 2015/10/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Forward_h
#define hifi_Controllers_Forward_h

namespace controller {

class Endpoint;
using EndpointPointer = std::shared_ptr<Endpoint>;
using EndpointList = std::list<EndpointPointer>;

class Filter;
using FilterPointer = std::shared_ptr<Filter>;
using FilterList = std::list<FilterPointer>;

class Route;
using RoutePointer = std::shared_ptr<Route>;
using RouteList = std::list<RoutePointer>;

class Conditional;
using ConditionalPointer = std::shared_ptr<Conditional>;
using ConditionalList = std::list<ConditionalPointer>;

class Mapping;
using MappingPointer = std::shared_ptr<Mapping>;
using MappingList = std::list<MappingPointer>;

struct Pose;
}

#endif
