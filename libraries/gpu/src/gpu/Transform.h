//
//  Transform.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 06/12/2016.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Transform_h
#define hifi_gpu_Transform_h

#include <vector>

#include <assert.h>

#include "Resource.h"

namespace gpu {

class TransformBuffer {
public:

    TransformBuffer() {}
    ~TransformBuffer() {}

protected:
    BufferPointer _buffer;
};
typedef std::shared_ptr<TransformBuffer> TransformBufferPointer;

};


#endif
