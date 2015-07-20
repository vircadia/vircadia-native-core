//
//  Query.h
//  interface/src/gpu
//
//  Created by Niraj Venkat on 7/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Query_h
#define hifi_gpu_Query_h

#include <assert.h>
#include <memory>
#include <vector>

#include "Format.h"

namespace gpu {

    class Query {
    public:
        Query();
        ~Query();

        uint32 queryResult;

        double getElapsedTime();

    protected:
        
        // This shouldn't be used by anything else than the Backend class with the proper casting.
        mutable GPUObject* _gpuObject = NULL;
        void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
        GPUObject* getGPUObject() const { return _gpuObject; }
        friend class Backend; 
    };
    
    typedef std::shared_ptr<Query> QueryPointer;
    typedef std::vector< QueryPointer > Queries;
};

#endif
