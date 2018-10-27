//
//  Created by Anthony Thibault 2018/11/09
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Controllers_Filters_Acceleration_Limiter_h
#define hifi_Controllers_Filters_Acceleration_Limiter_h

#include "../Filter.h"

namespace controller {

    class AccelerationLimiterFilter : public Filter {
        REGISTER_FILTER_CLASS(AccelerationLimiterFilter);

    public:
        AccelerationLimiterFilter() {}

        float apply(float value) const override { return value; }
        Pose apply(Pose value) const override;
        bool parseParameters(const QJsonValue& parameters) override;

    private:
        float _rotationAccelerationLimit { FLT_MAX };
        float _translationAccelerationLimit { FLT_MAX };
        float _rotationSnapThreshold { 0.0f };
        float _translationSnapThreshold { 0.0f };

        mutable glm::vec3 _prevPos[3];  // sensor space
        mutable glm::quat _prevRot[3];  // sensor space
        mutable glm::vec3 _unfilteredPrevPos[3];  // sensor space
        mutable glm::quat _unfilteredPrevRot[3];  // sensor space
        mutable bool _prevValid { false };
    };

}

#endif
