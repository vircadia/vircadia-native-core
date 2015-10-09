//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Endpoint_h
#define hifi_Controllers_Endpoint_h

#include <list>
#include <memory>

namespace Controllers {
    /*
    * Encapsulates a particular input / output,
    * i.e. Hydra.Button0, Standard.X, Action.Yaw
    */
    class Endpoint {
    public:
        virtual float value() = 0;
        virtual void apply(float newValue, float oldValue, const Endpoint& source) = 0;

        using Pointer = std::shared_ptr<Endpoint>;
        using List = std::list<Pointer>;

        static const List& getHardwareEndpoints();
    };

}

#endif
