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

#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class RotationAccumulator {
public:
    RotationAccumulator() {}

    uint32_t size() const { return _rotations.size(); }

    void add(const glm::quat& rotation) { _rotations.push_back(rotation); }

    glm::quat getAverage();

    void clear() { _rotations.clear(); }

private:
    std::vector<glm::quat> _rotations;
};

#endif // hifi_RotationAccumulator_h
