//
//  Created by Brad Hefta-Gaub 2017/04/11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_Rotate_h
#define hifi_Controllers_Filters_Rotate_h

#include <glm/gtx/transform.hpp>

#include "../Filter.h"

namespace controller {

class RotateFilter : public Filter {
    REGISTER_FILTER_CLASS(RotateFilter);
public:
    RotateFilter() { }
    RotateFilter(glm::quat rotation) : _rotation(rotation) {}

    virtual float apply(float value) const override { return value; }

    virtual Pose apply(Pose value) const override {
        return value.transform(glm::mat4(glm::quat(_rotation)));
    }

    virtual bool parseParameters(const QJsonValue& parameters) override { return parseQuatParameter(parameters, _rotation); }

private:
    glm::quat _rotation;
};

}

#endif
