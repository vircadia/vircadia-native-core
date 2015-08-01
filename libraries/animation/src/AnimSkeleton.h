//
//  AnimSkeleton.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimSkeleton
#define hifi_AnimSkeleton

#include <vector>

#include "FBXReader.h"

class AnimSkeleton {
public:
    typedef std::shared_ptr<AnimSkeleton> Pointer;
    AnimSkeleton(const std::vector<FBXJoint>& joints);
    int nameToJointIndex(const QString& jointName) const;
    int getNumJoints() const;
    AnimBone getBindPose(int jointIndex) const;
}

protected:
    std::vector<FBXJoint> _joints;
};

#endif
