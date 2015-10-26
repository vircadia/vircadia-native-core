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
};

inline glm::vec3 ColorUtils::toVec3(const xColor& color) {
    const float ONE_OVER_256 = 1.0f / 256.0f;
    return glm::vec3(color.red * ONE_OVER_256, color.green * ONE_OVER_256, color.blue * ONE_OVER_256);
}

#endif // hifi_ColorUtils_h