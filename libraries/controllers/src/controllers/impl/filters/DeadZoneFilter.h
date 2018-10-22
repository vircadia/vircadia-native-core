//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_DeadZoneFilter_h
#define hifi_Controllers_Filters_DeadZoneFilter_h

#include "../Filter.h"

namespace controller {

class DeadZoneFilter : public Filter {
    REGISTER_FILTER_CLASS(DeadZoneFilter);
public:
    DeadZoneFilter(float min = 0.0) : _min(min) {};

    virtual float apply(float value) const override;

    virtual Pose apply(Pose value) const override { return value; }

    virtual bool parseParameters(const QJsonValue& parameters) override;
protected:
    float _min = 0.0f;
};


}

#endif
