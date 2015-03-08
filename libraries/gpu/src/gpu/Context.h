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
#include "Shader.h"

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
        Vec4 _viewport;
    };

    template< typename T >
    static void setGPUObject(const Buffer& buffer, T* bo) {
       // buffer.setGPUObject(reinterpret_cast<GPUObject*>(bo));
        buffer.setGPUObject(bo);
    }
    template< typename T >
    static T* getGPUObject(const Buffer& buffer) {
        return reinterpret_cast<T*>(buffer.getGPUObject());
    }

    //void syncGPUObject(const Buffer& buffer);

    template< typename T >
    static void setGPUObject(const Texture& texture, T* to) {
        texture.setGPUObject(reinterpret_cast<GPUObject*>(to));
    }
    template< typename T >
    static T* getGPUObject(const Texture& texture) {
        return reinterpret_cast<T*>(texture.getGPUObject());
    }

    //void syncGPUObject(const Texture& texture);

    
    template< typename T >
    static void setGPUObject(const Shader& shader, T* so) {
        shader.setGPUObject(reinterpret_cast<GPUObject*>(so));
    }
    template< typename T >
    static T* getGPUObject(const Shader& shader) {
        return reinterpret_cast<T*>(shader.getGPUObject());
    }

  //  void syncGPUObject(const Shader& shader);

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
