//
//  ConicalViewFrustumData.h
//  libraries/shared/src/shared
//
//  Created by Nshan G. on 31 May 2022.
//  Copyright 2017 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_SHARED_SRC_SHARED_CONICALVIEWFRUSTUMUTILS_H
#define LIBRARIES_SHARED_SRC_SHARED_CONICALVIEWFRUSTUMUTILS_H

#include "../GLMHelpers.h"

const float DEFAULT_VIEW_ANGLE = 1.0f;
const float DEFAULT_VIEW_RADIUS = 10.0f;
const float DEFAULT_VIEW_FAR_CLIP = 100.0f;

struct ConicalViewFrustumData {
    glm::vec3 position { 0.0f, 0.0f, 0.0f };
    glm::vec3 direction { 0.0f, 0.0f, 1.0f };
    float angle { DEFAULT_VIEW_ANGLE };
    float radius { DEFAULT_VIEW_RADIUS };
    float farClip { DEFAULT_VIEW_FAR_CLIP };

    int serialize(unsigned char* destinationBuffer) const;
    int deserialize(const unsigned char* sourceBuffer);
    bool isVerySimilar(const ConicalViewFrustumData& other) const;
};

#endif /* end of include guard */
