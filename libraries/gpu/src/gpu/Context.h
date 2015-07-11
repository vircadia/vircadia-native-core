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

#include "Batch.h"

#include "Resource.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"

namespace gpu {

class Backend {
public:

    virtual~ Backend() {};
    virtual void render(Batch& batch) = 0;
    virtual void syncCache() = 0;

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
        Vec4 _viewport; // Public value is int but float in the shader to stay in floats for all the transform computations.
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

    template< typename T >
    static void setGPUObject(const Query& query, T* object) {
        query.setGPUObject(object);
    }
    template< typename T >
    static T* getGPUObject(const Query& query) {
        return reinterpret_cast<T*>(query.getGPUObject());
    }

protected:

};

class Context {
public:
    Context(Backend* backend);
    ~Context();

    void render(Batch& batch);

    void syncCache();

protected:
    Context(const Context& context);

    // This function can only be called by "static Shader::makeProgram()"
    // makeProgramShader(...) make a program shader ready to be used in a Batch.
    // It compiles the sub shaders, link them and defines the Slots and their bindings.
    // If the shader passed is not a program, nothing happens. 
    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings = Shader::BindingSet());

    std::unique_ptr<Backend> _backend;

    friend class Shader;
};


};

#endif
