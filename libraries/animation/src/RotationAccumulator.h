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
    RotationAccumulator() : _rotationSum(0.0f, 0.0f, 0.0f, 0.0f), _numRotations(0) { }

    int size() const { return _numRotations; }

    void add(glm::quat rotation);

    glm::quat getAverage();

    void clear();

private:
    glm::quat _rotationSum;
    int _numRotations;
};

#endif // hifi_RotationAccumulator_h
