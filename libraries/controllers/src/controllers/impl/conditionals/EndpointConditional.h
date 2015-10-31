//
//  Created by Bradley Austin Davis 2015/10/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_EndpointConditional_h
#define hifi_Controllers_EndpointConditional_h

#include "../Conditional.h"
#include "../Endpoint.h"

namespace controller {

class EndpointConditional : public Conditional {
public:
    EndpointConditional(Endpoint::Pointer endpoint) : _endpoint(endpoint) {}
    virtual bool satisfied() override { return _endpoint && _endpoint->value() != 0.0; }
private:
    Endpoint::Pointer _endpoint;
};

class NotEndpointConditional : public Conditional {
public:
    NotEndpointConditional(Endpoint::Pointer endpoint) : _endpoint(endpoint) {}
    virtual bool satisfied() override { return _endpoint && _endpoint->value() == 0.0; }
private:
    Endpoint::Pointer _endpoint;
};

}
#endif
