//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MySkeletonModel_h
#define hifi_MySkeletonModel_h

#include <avatars-renderer/SkeletonModel.h>
#include <AnimUtil.h>
#include "MyAvatar.h"

/// A skeleton loaded from a model.
class MySkeletonModel : public SkeletonModel {
    Q_OBJECT

private:
    using Parent = SkeletonModel;

public:
    MySkeletonModel(Avatar* owningAvatar, QObject* parent = nullptr);
    void updateRig(float deltaTime, glm::mat4 parentTransform) override;

private:
    void updateFingers();

    CriticallyDampedSpringPoseHelper _smoothHipsHelper;  // sensor frame
    bool _prevIsFlying { false };
    float _flyIdleTimer { 0.0f };

    float _prevIsEstimatingHips { false };

    std::map<int, int> _jointRotationFrameOffsetMap;
};

#endif // hifi_MySkeletonModel_h
