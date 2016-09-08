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

class ViewFrustum;

class LightClusters {
public:
    
    LightStagePointer _lightStage;

    gpu::BufferPointer _lightIndicesBuffer;
};

#endif
