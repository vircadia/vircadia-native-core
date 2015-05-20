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

#include "Resource.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"

namespace gpu {

class GPUObject {
public:
    GPUObject() {}
    virtual ~GPUObject() {}
};

class Batch;

class Backend {
public:

    class TransformObject {
    public:
        Mat4 _model;
        Mat4 _modelInverse;
    };

    class TransformCamera {
    public:
        Mat4 _view;
        Mat4 _viewInverse;
        Mat4 _projectionViewUntranslated;
        Mat4 _projection;
        Mat4 _projectionInverse;
        Vec4 _viewport;
    };

    template< typename T >
    static void setGPUObject(const Buffer& buffer, T* object) {
        buffer.setGPUObject(object);
    }
    template< typename T >
    static T* getGPUObject(const Buffer& buffer) {
        return reinterpret_cast<T*>(buffer.getGPUObject());
    }

    template< typename T >
    static void setGPUObject(const Texture& texture, T* object) {
        texture.setGPUObject(object);
    }
    template< typename T >
    static T* getGPUObject(const Texture& texture) {
        return reinterpret_cast<T*>(texture.getGPUObject());
    }
    
    template< typename T >
    static void setGPUObject(const Shader& shader, T* object) {
        shader.setGPUObject(object);
    }
    template< typename T >
    static T* getGPUObject(const Shader& shader) {
        return reinterpret_cast<T*>(shader.getGPUObject());
    }

    template< typename T >
    static void setGPUObject(const Pipeline& pipeline, T* object) {
        pipeline.setGPUObject(object);
    }
    template< typename T >
    static T* getGPUObject(const Pipeline& pipeline) {
        return reinterpret_cast<T*>(pipeline.getGPUObject());
    }

    template< typename T >
    static void setGPUObject(const State& state, T* object) {
        state.setGPUObject(object);
    }
    template< typename T >
    static T* getGPUObject(const State& state) {
        return reinterpret_cast<T*>(state.getGPUObject());
    }

    template< typename T >
    static void setGPUObject(const Framebuffer& framebuffer, T* object) {
        framebuffer.setGPUObject(object);
    }
    template< typename T >
    static T* getGPUObject(const Framebuffer& framebuffer) {
        return reinterpret_cast<T*>(framebuffer.getGPUObject());
    }

protected:

};

class Context {
public:
    Context();
    Context(const Context& context);
    ~Context();

    void enqueueBatch(Batch& batch);



protected:

    // This function can only be called by "static Shader::makeProgram()"
    // makeProgramShader(...) make a program shader ready to be used in a Batch.
    // It compiles the sub shaders, link them and defines the Slots and their bindings.
    // If the shader passed is not a program, nothing happens. 
    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings = Shader::BindingSet());

    friend class Shader;
};


};

#endif
