//
//  AABoxData.h
//  libraries/shared/src
//
//  Created by Nshan G. on 3 July 2022
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef LIBRARIES_SHARED_SRC_AABOXDARA_H
#define LIBRARIES_SHARED_SRC_AABOXDARA_H

#include <glm/glm.hpp>

struct AABoxData {
    glm::vec3 corner;
    glm::vec3 scale;
};

inline bool isTouching(AABoxData one, AABoxData other) {
    glm::vec3 relativeCenter = one.corner - other.corner + ((one.scale - other.scale) * 0.5f);

    glm::vec3 totalHalfScale = (one.scale + other.scale) * 0.5f;

    return fabsf(relativeCenter.x) <= totalHalfScale.x &&
        fabsf(relativeCenter.y) <= totalHalfScale.y &&
        fabsf(relativeCenter.z) <= totalHalfScale.z;
}

#endif /* end of include guard */
