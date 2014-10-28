//
//  Context.h
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Context_h
#define hifi_gpu_Context_h

#include <assert.h>
#include "InterfaceConfig.h"



namespace gpu {

class Batch;
class Buffer;

class Backend {
public:
    class GPUObject {
    public:
        GPUObject() {}
        ~GPUObject() {}
    };

    template< typename T >
    static void setGPUObject(const Buffer& buffer, T* bo) {
        buffer.setGPUObject(reinterpret_cast<GPUObject*>(bo));
    }
    template< typename T >
    static T* getGPUObject(const Buffer& buffer) {
        return reinterpret_cast<T*>(buffer.getGPUObject());
    }

    void syncGPUObject(const Buffer& buffer);

protected:

};

class Context {
public:
    Context();
    Context(const Context& context);
    ~Context();

    void enqueueBatch(Batch& batch);

protected:

};


};

#endif
