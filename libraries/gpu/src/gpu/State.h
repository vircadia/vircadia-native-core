//
//  Pipeline.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_State_h
#define hifi_gpu_State_h

#include "Format.h"
#include <vector>
#include <QSharedPointer>
 

namespace gpu {

class GPUObject;

class State {
public:
    State() {}
    virtual ~State();

    // Work in progress, not used
    /*
    enum Field {
        FILL_MODE,
        CULL_MODE,
        DEPTH_BIAS,
        DEPTH_BIAS_CLAMP,
        DEPTH_BIASSLOPE_SCALE,

        FRONT_CLOCKWISE,
        DEPTH_CLIP_ENABLE,
        SCISSR_ENABLE,
        MULTISAMPLE_ENABLE,
        ANTIALISED_LINE_ENABLE,

        DEPTH_ENABLE,
        DEPTH_WRITE_MASK,
        DEPTH_FUNCTION,

        STENCIL_ENABLE,
        STENCIL_READ_MASK,
        STENCIL_WRITE_MASK,
        STENCIL_FUNCTION_FRONT,
        STENCIL_FUNCTION_BACK,
        STENCIL_REFERENCE,

        BLEND_INDEPENDANT_ENABLE,
        BLEND_ENABLE,
        BLEND_SOURCE,
        BLEND_DESTINATION,
        BLEND_OPERATION,
        BLEND_SOURCE_ALPHA,
        BLEND_DESTINATION_ALPHA,
        BLEND_OPERATION_ALPHA,
        BLEND_WRITE_MASK,
        BLEND_FACTOR,

        SAMPLE_MASK,

        ALPHA_TO_COVERAGE_ENABLE,
    };
    */

protected:
    State(const State& state);
    State& operator=(const State& state);

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = NULL;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef QSharedPointer< State > StatePointer;
typedef std::vector< StatePointer > States;

};


#endif
