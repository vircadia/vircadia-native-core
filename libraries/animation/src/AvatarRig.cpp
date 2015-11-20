//
//  AvatarRig.cpp
//  libraries/animation/src/
//
//  Created by SethAlves on 2015-7-22.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarRig.h"

void AvatarRig::setJointTranslation(int index, bool valid, const glm::vec3& translation, float priority) {
    if (index >= 0 && index < (int)_overrideFlags.size()) {
        if (valid) {
            assert(_overrideFlags.size() == _overridePoses.size());
            _overrideFlags[index] = true;
            _overridePoses[index].trans = translation;
        }
    }
}

void AvatarRig::setJointState(int index, bool valid, const glm::quat& rotation, const glm::vec3& translation, float priority) {
    if (index >= 0 && index < (int)_overrideFlags.size()) {
        assert(_overrideFlags.size() == _overridePoses.size());
        _overrideFlags[index] = true;
        _overridePoses[index].rot = rotation;
        _overridePoses[index].trans = translation;
    }
}
