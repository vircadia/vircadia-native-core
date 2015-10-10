//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filter_h
#define hifi_Controllers_Filter_h

#include <list>
#include <memory>

namespace Controllers {

    // Encapsulates part of a filter chain
    class Filter {
    public:
        virtual float apply(float newValue, float oldValue) = 0;

        using Pointer = std::shared_ptr<Filter>;
        using List = std::list<Pointer>;
    };

}

#endif
