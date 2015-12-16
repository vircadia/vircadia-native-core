//
//  Created by Bradley Austin Davis 2015/10/25
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_Scale_h
#define hifi_Controllers_Filters_Scale_h

#include "../Filter.h"

namespace controller {

class ScaleFilter : public Filter {
    REGISTER_FILTER_CLASS(ScaleFilter);
public:
    ScaleFilter() {}
    ScaleFilter(float scale) : _scale(scale) {}

    virtual float apply(float value) const override {
        return value * _scale;
    }
    virtual bool parseParameters(const QJsonValue& parameters) override;

private:
    float _scale = 1.0f;
};

}

#endif
