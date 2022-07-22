//
//  GLMHelpers.cpp
//  libraries/vircadia-client/src/internal/avatars
//
//  Created by Nshan G. on 28 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_GLMHELPERS_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_GLMHELPERS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "../../avatars.h"

namespace vircadia::client
{

    inline glm::vec3 glmVec3From(vircadia_vector v) {
        return {v.x, v.y, v.z};
    }

    inline glm::quat glmQuatFrom(vircadia_quaternion q) {
        return {q.w, q.x, q.y, q.z};
    }

    inline vircadia_vector vectorFrom(glm::vec3 v) {
        return {v.x, v.y, v.z};
    }

    inline vircadia_quaternion quaternionFrom(glm::quat q) {
        return {q.x, q.y, q.z, q.w};
    }

    template <typename PackedArray,
        std::enable_if_t<!std::is_same_v<PackedArray, glm::vec3>>* = nullptr>
    vircadia_vector vectorFrom(PackedArray array) {
        return {array.data[0], array.data[1], array.data[2]};
    }

    template <typename PackedArray,
        std::enable_if_t<!std::is_same_v<PackedArray, glm::quat>>* = nullptr>
    vircadia_quaternion quaternionFrom(PackedArray array) {
        return {array.data[1], array.data[2], array.data[3], array.data[0]};
    }

    template <typename PackedArray>
    void serialize(vircadia_vector v, PackedArray& array) {
        array.data[0] = v.x;
        array.data[1] = v.y;
        array.data[2] = v.z;
    }

    template <typename PackedArray>
    void serialize(vircadia_quaternion q, PackedArray& array) {
        array.data[0] = q.w;
        array.data[1] = q.x;
        array.data[2] = q.y;
        array.data[3] = q.z;
    }

} // namespace vircadia::client

#endif /* end of include guard */
