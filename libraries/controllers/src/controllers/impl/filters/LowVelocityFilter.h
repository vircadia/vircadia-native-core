//
//  Created by Dante Ruiz 2017/05/15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Controllers_Filters_Low_Velocity_h
#define hifi_Controllers_Filters_Low_Velocity_h

#include "../Filter.h"

namespace controller {

    class LowVelocityFilter : public Filter {
        REGISTER_FILTER_CLASS(LowVelocityFilter);

    public:
        LowVelocityFilter() {}
        LowVelocityFilter(float rotationConstant, float translationConstant) :
            _translationConstant(translationConstant), _rotationConstant(rotationConstant) {}

        float apply(float value) const override { return value; }
        Pose apply(Pose newPose) const override;
        bool parseParameters(const QJsonValue& parameters) override;

    private:
        float _translationConstant { 0.1f };
        float _rotationConstant { 0.1f };
        mutable Pose _oldPose { Pose() };
    };

}

#endif
