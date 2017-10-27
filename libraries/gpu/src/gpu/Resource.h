//
//  Resource.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/8/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Resource_h
#define hifi_gpu_Resource_h

#include "Forward.h"

namespace gpu {

class Resource {
public:
    using Size = gpu::Size;
    static const Size NOT_ALLOCATED = INVALID_SIZE;

    // The size in bytes of data stored in the resource
    virtual Size getSize() const = 0;

    enum Type {
        BUFFER = 0,
        TEXTURE_1D,
        TEXTURE_2D,
        TEXTURE_3D,
        TEXTURE_CUBE,
        TEXTURE_1D_ARRAY,
        TEXTURE_2D_ARRAY,
        TEXTURE_3D_ARRAY,
        TEXTURE_CUBE_ARRAY,
    };

protected:
    Resource();
    virtual ~Resource();
}; // Resource

}


// FIXME Compatibility with headers that rely on Resource.h for the buffer and buffer view definitions
#include "Buffer.h"


#endif
