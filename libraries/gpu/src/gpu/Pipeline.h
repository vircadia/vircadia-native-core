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
    static Pipeline* create(const ShaderPointer& program, const StatePointer& state);
    ~Pipeline();

    const ShaderPointer& getProgram() const { return _program; }

    const StatePointer& getState() const { return _state; }

protected:
    ShaderPointer _program;
    StatePointer _state;

    Pipeline();
    Pipeline(const Pipeline& pipeline); // deep copy of the sysmem shader
    Pipeline& operator=(const Pipeline& pipeline); // deep copy of the sysmem texture

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = nullptr;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef std::shared_ptr< Pipeline > PipelinePointer;
typedef std::vector< PipelinePointer > Pipelines;

};


#endif
