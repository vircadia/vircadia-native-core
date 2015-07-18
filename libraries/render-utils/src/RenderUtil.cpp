//
//  RenderUtil.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 8/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <DependencyManager.h>
#include "GeometryCache.h"

#include "RenderUtil.h"

void renderFullscreenQuad(float sMin, float sMax, float tMin, float tMax) {
    glm::vec4 color(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec2 topLeft(-1.0f, -1.0f);
    glm::vec2 bottomRight(1.0f, 1.0f);
    glm::vec2 texCoordTopLeft(sMin, tMin);
    glm::vec2 texCoordBottomRight(sMax, tMax);

    DependencyManager::get<GeometryCache>()->renderQuad(topLeft, bottomRight, texCoordTopLeft, texCoordBottomRight, color);
}
