//
//  IKTarget.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_IKTarget_h
#define hifi_IKTarget_h

#include "AnimSkeleton.h"

class IKTarget {
public:
    enum class Type {
        RotationAndPosition,
        RotationOnly,
        HmdHead,
        HipsRelativeRotationAndPosition,
        Unknown,
    };

    IKTarget() {}

    const glm::vec3& getTranslation() const { return _pose.trans; }
    const glm::quat& getRotation() const { return _pose.rot; }
    int getIndex() const { return _index; }
    Type getType() const { return _type; }

    void setPose(const glm::quat& rotation, const glm::vec3& translation);
    void setIndex(int index) { _index = index; }
    void setType(int);

private:
    AnimPose _pose;
    int _index = -1;
    Type _type = Type::RotationAndPosition;

};

#endif // hifi_IKTarget_h
