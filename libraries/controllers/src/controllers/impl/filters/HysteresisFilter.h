//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_Hysteresis_h
#define hifi_Controllers_Filters_Hysteresis_h

#include "../Filter.h"

namespace controller {

class HysteresisFilter : public Filter {
    REGISTER_FILTER_CLASS(HysteresisFilter);
public:
    HysteresisFilter(float min = 0.25, float max = 0.75);
    virtual float apply(float value) const override;
    virtual bool parseParameters(const QJsonValue& parameters) override;
protected:
    float _min;
    float _max;
    mutable bool _signaled { false };
};

}

#endif
