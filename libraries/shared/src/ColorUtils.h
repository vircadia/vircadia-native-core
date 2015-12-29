//
//  ColorUtils.h
//  libraries/shared/src
//
//  Created by Sam Gateau on 10/24/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ColorUtils_h
#define hifi_ColorUtils_h

#include <glm/glm.hpp>
#include <SharedUtil.h>

#include "DependencyManager.h"

class ColorUtils {
public:
    inline static glm::vec3 toVec3(const xColor& color);

    // Convert from gamma 2.2 space to linear
    inline static glm::vec3 toLinearVec3(const glm::vec3& srgb);
    inline static glm::vec3 toGamma22Vec3(const glm::vec3& linear);
};

inline glm::vec3 ColorUtils::toVec3(const xColor& color) {
    const float ONE_OVER_255 = 1.0f / 255.0f;
    return glm::vec3(color.red * ONE_OVER_255, color.green * ONE_OVER_255, color.blue * ONE_OVER_255);
}

inline glm::vec3 ColorUtils::toLinearVec3(const glm::vec3& srgb) {
    const float GAMMA_22 = 2.2f;
    // Couldn't find glm::pow(vec3, vec3) ? so did it myself...
    return glm::vec3(glm::pow(srgb.x, GAMMA_22), glm::pow(srgb.y, GAMMA_22), glm::pow(srgb.z, GAMMA_22));
}

inline glm::vec3 ColorUtils::toGamma22Vec3(const glm::vec3& linear) {
    const float INV_GAMMA_22 = 1.0f / 2.2f;
    // Couldn't find glm::pow(vec3, vec3) ? so did it myself...
    return glm::vec3(glm::pow(linear.x, INV_GAMMA_22), glm::pow(linear.y, INV_GAMMA_22), glm::pow(linear.z, INV_GAMMA_22));
}

#endif // hifi_ColorUtils_h