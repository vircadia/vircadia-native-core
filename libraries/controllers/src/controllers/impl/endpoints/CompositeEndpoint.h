//
//  Created by Bradley Austin Davis 2015/10/23
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_CompositeEndpoint_h
#define hifi_Controllers_CompositeEndpoint_h

#include "../Endpoint.h"

namespace controller {
    class CompositeEndpoint : public Endpoint, Endpoint::Pair {
    public:
        using Endpoint::apply;
        CompositeEndpoint(Endpoint::Pointer first, Endpoint::Pointer second);

        virtual float peek() const override;
        virtual float value() override;
        virtual void apply(float newValue, const Pointer& source) override;
        virtual bool readable() const override;

    };

}

#endif
