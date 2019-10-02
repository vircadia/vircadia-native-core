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

#include <glm/gtc/matrix_transform.hpp>

#include "../Filter.h"

namespace controller {

class ScaleFilter : public Filter {
    REGISTER_FILTER_CLASS(ScaleFilter);
public:
    ScaleFilter() = default;
    ScaleFilter(float scale) : _scale(scale) {}

    virtual AxisValue apply(AxisValue value) const override {
        return { value.value * _scale, value.timestamp, value.valid };
    }

    virtual Pose apply(Pose value) const override {
        return value.transform(glm::scale(glm::mat4(), glm::vec3(_scale)));
    }

    virtual bool parseParameters(const QJsonValue& parameters) override;

private:
    float _scale = 1.0f;
};

}

#endif
