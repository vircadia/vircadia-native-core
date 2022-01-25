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
    /*@jsdoc
     * <p>An IK target type.</p>
     * <table>
     *   <thead>
     *     <tr><th>Value</th><th>Name</th><th>Description</th>
     *   </thead>
     *   <tbody>
     *     <tr><td><code>0</code></td><td>RotationAndPosition</td><td>Attempt to reach the rotation and position end 
     *       effector.</td></tr>
     *     <tr><td><code>1</code></td><td>RotationOnly</td><td>Attempt to reach the end effector rotation only.</td></tr>
     *     <tr><td><code>2</code></td><td>HmdHead</td><td>
     *       <p>A special mode of IK that would attempt to prevent unnecessary bending of the spine.</p>
     *       <p class="important">Deprecated: This target type is deprecated and will be removed.</p></td></tr>
     *     <tr><td><code>3</code></td><td>HipsRelativeRotationAndPosition</td><td>Attempt to reach a rotation and position end 
     *       effector that is not in absolute rig coordinates but is offset by the avatar hips translation.</td></tr>
     *     <tr><td><code>4</code></td><td>Spline</td><td>Use a cubic Hermite spline to model the human spine. This prevents 
     *       kinks in the spine and allows for a small amount of stretch and squash.</td></tr>
     *     <tr><td><code>5</code></td><td>Unknown</td><td>IK is disabled.</td></tr>
     *   </tbody>
     * </table>
     * @typedef {number} MyAvatar.IKTargetType
     */
    enum class Type {
        RotationAndPosition,
        RotationOnly,
        HmdHead,
        HipsRelativeRotationAndPosition,
        Spline,
        Unknown
    };

    IKTarget() {}

    const glm::vec3& getTranslation() const { return _pose.trans(); }
    const glm::quat& getRotation() const { return _pose.rot(); }
    const AnimPose& getPose() const { return _pose; }
    glm::vec3 getPoleVector() const { return _poleVector; }
    glm::vec3 getPoleReferenceVector() const { return _poleReferenceVector; }
    bool getPoleVectorEnabled() const { return _poleVectorEnabled; }
    int getIndex() const { return _index; }
    Type getType() const { return _type; }
    int getNumFlexCoefficients() const { return (int)_numFlexCoefficients; }
    float getFlexCoefficient(size_t chainDepth) const;

    void setPose(const glm::quat& rotation, const glm::vec3& translation);
    void setPoleVector(const glm::vec3& poleVector) { _poleVector = poleVector; }
    void setPoleReferenceVector(const glm::vec3& poleReferenceVector) { _poleReferenceVector = poleReferenceVector; }
    void setPoleVectorEnabled(bool poleVectorEnabled) { _poleVectorEnabled = poleVectorEnabled; }
    void setIndex(int index) { _index = index; }
    void setType(int);
    void setFlexCoefficients(size_t numFlexCoefficientsIn, const float* flexCoefficientsIn);

    void setWeight(float weight) { _weight = weight; }
    float getWeight() const { return _weight; }

    enum FlexCoefficients { MAX_FLEX_COEFFICIENTS = 10 };

private:
    AnimPose _pose;
    glm::vec3 _poleVector;
    glm::vec3 _poleReferenceVector;
    bool _poleVectorEnabled { false };
    int _index { -1 };
    Type _type { Type::Unknown };
    float _weight { 0.0f };
    float _flexCoefficients[MAX_FLEX_COEFFICIENTS];
    size_t _numFlexCoefficients;
};

#endif // hifi_IKTarget_h
