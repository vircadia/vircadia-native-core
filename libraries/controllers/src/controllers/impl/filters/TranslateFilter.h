//
//  Created by Brad Hefta-Gaub 2017/04/11
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_Filters_Translate_h
#define hifi_Controllers_Filters_Translate_h

#include <glm/gtx/transform.hpp>

#include "../Filter.h"

namespace controller {

class TranslateFilter : public Filter {
    REGISTER_FILTER_CLASS(TranslateFilter);
public:
    TranslateFilter() { }
    TranslateFilter(glm::vec3 translate) : _translate(translate) {}

    virtual float apply(float value) const override { return value; }
    virtual Pose apply(Pose value) const override { return value.transform(glm::translate(_translate)); }
    virtual bool parseParameters(const QJsonValue& parameters) override { return parseVec3Parameter(parameters, _translate); }

private:
    glm::vec3 _translate { 0.0f };
};

}

#endif
