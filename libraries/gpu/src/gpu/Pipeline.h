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
#ifndef hifi_gpu_Pipeline_h
#define hifi_gpu_Pipeline_h

#include "Resource.h"
#include <memory>
#include <set>

#include "Shader.h"
#include "State.h"
 
namespace gpu {

class Pipeline {
public:
    using Pointer = std::shared_ptr< Pipeline >;

    static Pointer create(const ShaderPointer& program, const StatePointer& state);
    ~Pipeline();

    const ShaderPointer& getProgram() const { return _program; }

    const StatePointer& getState() const { return _state; }

    const GPUObjectPointer gpuObject {};
    
protected:
    ShaderPointer _program;
    StatePointer _state;

    Pipeline();
    Pipeline(const Pipeline& pipeline); // deep copy of the sysmem shader
    Pipeline& operator=(const Pipeline& pipeline); // deep copy of the sysmem texture
};

typedef Pipeline::Pointer PipelinePointer;
typedef std::vector< PipelinePointer > Pipelines;

};


#endif
