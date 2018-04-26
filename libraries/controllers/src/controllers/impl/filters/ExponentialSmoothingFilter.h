//
//  Created by Anthony Thibault 2017/12/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Controllers_Filters_Exponential_Smoothing_h
#define hifi_Controllers_Filters_Exponential_Smoothing_h

#include "../Filter.h"

namespace controller {

    class ExponentialSmoothingFilter : public Filter {
        REGISTER_FILTER_CLASS(ExponentialSmoothingFilter);

    public:
        ExponentialSmoothingFilter() {}
        ExponentialSmoothingFilter(float rotationConstant, float translationConstant) :
            _translationConstant(translationConstant), _rotationConstant(rotationConstant) {}

        float apply(float value) const override { return value; }
        Pose apply(Pose value) const override;
        bool parseParameters(const QJsonValue& parameters) override;

    private:

        // Constant between 0 and 1.
        // 1 indicates no smoothing at all, poses are passed through unaltered.
        // Values near 1 are less smooth with lower latency.
        // Values near 0 are more smooth with higher latency.
        float _translationConstant { 0.375f };
        float _rotationConstant { 0.375f };

        mutable Pose _prevSensorValue { Pose() };  // sensor space
    };

}

#endif
