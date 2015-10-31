//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_Clamp_h
#define hifi_Controllers_Filters_Clamp_h

#include "../Filter.h"

namespace controller {

class ClampFilter : public Filter {
    REGISTER_FILTER_CLASS(ClampFilter);
public:
    ClampFilter(float min = 0.0, float max = 1.0) : _min(min), _max(max) {};
    virtual float apply(float value) const override {
        return glm::clamp(value, _min, _max);
    }
    virtual bool parseParameters(const QJsonValue& parameters) override;
protected:
    float _min = 0.0f;
    float _max = 1.0f;
};

}

#endif
