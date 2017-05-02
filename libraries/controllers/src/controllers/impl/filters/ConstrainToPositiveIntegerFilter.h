//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_ConstrainToPositiveInteger_h
#define hifi_Controllers_Filters_ConstrainToPositiveInteger_h

#include "../Filter.h"

namespace controller {

class ConstrainToPositiveIntegerFilter : public Filter {
    REGISTER_FILTER_CLASS(ConstrainToPositiveIntegerFilter);
public:
    ConstrainToPositiveIntegerFilter() {};

    virtual float apply(float value) const override {
        return (value <= 0.0f) ? 0.0f : 1.0f;
    }

    virtual Pose apply(Pose value) const override { return value; }

protected:
};

}

#endif
