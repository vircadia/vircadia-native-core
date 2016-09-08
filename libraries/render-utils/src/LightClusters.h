//
//  LightClusters.h
//
//  Created by Sam Gateau on 9/7/2016.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_render_utils_LightClusters_h
#define hifi_render_utils_LightClusters_h

#include "gpu/Framebuffer.h"

#include "LightStage.h"

#include <ViewFrustum.h>
/*
class FrustumGrid {
public:
    float _near { 0.1f };
    float _nearPrime { 1.0f };
    float _farPrime { 400.0f };
    float _far { 10000.0f };

    glm::uvec3 _dims { 16, 16, 16 };

    float viewToLinearDepth(float depth) const {
        float nDepth = -depth;
        float ldepth = (nDepth - _nearPrime) / (_farPrime - _nearPrime);

        if (ldepth < 0.0f) {
            return (nDepth - _near) / (_nearPrime - _near) - 1.0f;
        }
        if (ldepth > 1.0f) {
            return (nDepth - _farPrime) / (_far - _farPrime) + 1.0f;
        }
        return ldepth;
    }

    float linearToGridDepth(float depth) const {
        return depth / (float) _dims.z;
    }

    glm::vec2 linearToGridXY(const glm::vec3& cpos) const {
        return glm::vec2(cpos.x / (float) _dims.x, cpos.y / (float) _dims.y);
    }

    int gridDepthToLayer(float gridDepth) const {
        return (int) gridDepth;
    }

    glm::ivec3 viewToGridPos(const glm::vec3& pos) const {
        
        return glm::ivec3(0);
    }

};
*/
class LightClusters {
public:
    
    LightClusters();


    void updateFrustum(const ViewFrustum& frustum);

    

    ViewFrustum _frustum;

    LightStagePointer _lightStage;

    gpu::BufferPointer _lightIndicesBuffer;
};

#endif
