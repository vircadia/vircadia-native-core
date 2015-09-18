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
#include <mutex>

#include <GLMHelpers.h>

#include "Batch.h"

#include "Resource.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"

class QImage;

namespace gpu {

struct StereoState {
    bool _enable{ false };
    bool _skybox{ false };
    // 0 for left eye, 1 for right eye
    uint8_t _pass{ 0 };
    mat4 _eyeViews[2];
    mat4 _eyeProjections[2];
};

class Backend {
public:
    virtual~ Backend() {};

    virtual void render(Batch& batch) = 0;
    virtual void enableStereo(bool enable) {
        _stereo._enable = enable;
    }

    virtual bool isStereo() {
        return _stereo._enable;
    }

    void setStereoProjections(const mat4 eyeProjections[2]) {
        for (int i = 0; i < 2; ++i) {
            _stereo._eyeProjections[i] = eyeProjections[i];
        }
    }

    void setStereoViews(const mat4 views[2]) {
        for (int i = 0; i < 2; ++i) {
            _stereo._eyeViews[i] = views[i];
        }
    }

    void getStereoProjections(mat4* eyeProjections) const {
        for (int i = 0; i < 2; ++i) {
            eyeProjections[i] = _stereo._eyeProjections[i];
        }
    }

    void getStereoViews(mat4* eyeViews) const {
        for (int i = 0; i < 2; ++i) {
            eyeViews[i] = _stereo._eyeViews[i];
        }
    }

    virtual void syncCache() = 0;
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) = 0;

    // UBO class... layout MUST match the layout in TransformCamera.slh
    class TransformObject {
    public:
        Mat4 _model;
        Mat4 _modelInverse;
    };

    // UBO class... layout MUST match the layout in TransformCamera.slh
    class TransformCamera {
    public:
        Mat4 _view;
        mutable Mat4 _viewInverse;
        mutable Mat4 _projectionViewUntranslated;
        Mat4 _projection;
        mutable Mat4 _projectionInverse;
        Vec4 _viewport; // Public value is int but float in the shader to stay in floats for all the transform computations.

        const Backend::TransformCamera& recomputeDerived() const;
        TransformCamera getEyeCamera(int eye, const StereoState& stereo) const;
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
    StereoState  _stereo;
};

class Context {
public:
    typedef Backend* (*CreateBackend)();
    typedef bool (*MakeProgram)(Shader& shader, const Shader::BindingSet& bindings);


    // This one call must happen before any context is created or used (Shader::MakeProgram) in order to setup the Backend and any singleton data needed
    template <class T>
    static void init() {
        std::call_once(_initialized, [] {
            _createBackendCallback = T::createBackend;
            _makeProgramCallback = T::makeProgram;
            T::init();
        });
    }

    Context();
    ~Context();

    void render(Batch& batch);
    void enableStereo(bool enable = true);
    bool isStereo();
    void setStereoProjections(const mat4 eyeProjections[2]);
    void setStereoViews(const mat4 eyeViews[2]);
    void getStereoProjections(mat4* eyeProjections) const;
    void getStereoViews(mat4* eyeViews) const;
    void syncCache();

    // Downloading the Framebuffer is a synchronous action that is not efficient.
    // It s here for convenience to easily capture a snapshot
    void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage);

protected:
    Context(const Context& context);

    std::unique_ptr<Backend> _backend;

    // This function can only be called by "static Shader::makeProgram()"
    // makeProgramShader(...) make a program shader ready to be used in a Batch.
    // It compiles the sub shaders, link them and defines the Slots and their bindings.
    // If the shader passed is not a program, nothing happens. 
    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings);

    static CreateBackend _createBackendCallback;
    static MakeProgram _makeProgramCallback;
    static std::once_flag _initialized;

    friend class Shader;
};
typedef std::shared_ptr<Context> ContextPointer;

};

#endif
