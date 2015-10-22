//
//  RotationAccumulator.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RotationAccumulator_h
#define hifi_RotationAccumulator_h

#include <glm/gtc/quaternion.hpp>

class RotationAccumulator {
public:
    RotationAccumulator() : _rotationSum(0.0f, 0.0f, 0.0f, 0.0f), _numRotations(0), _isDirty(false) { }

    int size() const { return _numRotations; }

    /// \param rotation rotation to add
    /// \param weight contribution factor of this rotation to total accumulation
    void add(const glm::quat& rotation, float weight = 1.0f);

    glm::quat getAverage();

    /// \return true if any rotations were accumulated
    bool isDirty() const { return _isDirty; }

    /// \brief clear accumulated rotation but don't change _isDirty
    void clear();

    /// \brief clear accumulated rotation and set _isDirty to false
    void clearAndClean();

private:
    glm::quat _rotationSum;
    int _numRotations;
    bool _isDirty;
};

#endif // hifi_RotationAccumulator_h
