//
//  Created by Brad Hefta-Gaub 2017/04/11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_PostTransform_h
#define hifi_Controllers_Filters_PostTransform_h

#include <glm/gtx/transform.hpp>

#include "../Filter.h"

namespace controller {

class PostTransformFilter : public Filter {
    REGISTER_FILTER_CLASS(PostTransformFilter);
public:
    PostTransformFilter() { }
    PostTransformFilter(glm::mat4 transform) : _transform(transform) {}
    virtual float apply(float value) const override { return value; }
    virtual Pose apply(Pose value) const override { return value.postTransform(_transform); }
    virtual bool parseParameters(const QJsonValue& parameters) override { return parseMat4Parameter(parameters, _transform); }
private:
    glm::mat4 _transform;
};

}

#endif
