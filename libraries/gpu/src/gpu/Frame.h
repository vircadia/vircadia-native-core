//
//  Created by Bradley Austin Davis on 2016/07/26
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Frame_h
#define hifi_gpu_Frame_h

#include "Forward.h"

namespace gpu {

class Frame {
public:
    /// The sensor pose used for rendering the frame, only applicable for HMDs
    glm::mat4 pose;
    /// The collection of batches which make up the frame
    std::vector<Batch> batches;
    /// The destination framebuffer in which the results will be placed
    FramebufferPointer framebuffer;
};

};


#endif
