//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_ConstrainToIntegerFilter_h
#define hifi_Controllers_Filters_ConstrainToIntegerFilter_h

#include "../Filter.h"

namespace controller {

class ConstrainToIntegerFilter : public Filter {
    REGISTER_FILTER_CLASS(ConstrainToIntegerFilter);
public:
    ConstrainToIntegerFilter() {};

    virtual float apply(float value) const override {
        return glm::sign(value);
    }
protected:
};

}

#endif
