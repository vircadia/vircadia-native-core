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

const float HACK_HMD_TARGET_WEIGHT = 8.0f;

class IKTarget {
public:
    enum class Type {
        RotationAndPosition,
        RotationOnly,
        HmdHead,
        HipsRelativeRotationAndPosition,
        Unknown
    };

    IKTarget() {}

    const glm::vec3& getTranslation() const { return _pose.trans(); }
    const glm::quat& getRotation() const { return _pose.rot(); }
    const AnimPose& getPose() const { return _pose; }
    int getIndex() const { return _index; }
    Type getType() const { return _type; }

    void setPose(const glm::quat& rotation, const glm::vec3& translation);
    void setIndex(int index) { _index = index; }
    void setType(int);
    void setFlexCoefficients(size_t numFlexCoefficientsIn, const float* flexCoefficientsIn);
    float getFlexCoefficient(size_t chainDepth) const;

    void setWeight(float weight) { _weight = weight; }
    float getWeight() const { return _weight; }

    enum FlexCoefficients { MAX_FLEX_COEFFICIENTS = 10 };

private:
    AnimPose _pose;
    int _index{-1};
    Type _type{Type::RotationAndPosition};
    float _weight;
    float _flexCoefficients[MAX_FLEX_COEFFICIENTS];
    size_t _numFlexCoefficients;
};

#endif // hifi_IKTarget_h
