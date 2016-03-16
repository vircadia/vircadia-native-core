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
    class TransformCamera {
    public:
        mutable Mat4 _view;
        mutable Mat4 _viewInverse;
        mutable Mat4 _projectionViewUntranslated;
        Mat4 _projection;
        mutable Mat4 _projectionInverse;
        Vec4 _viewport; // Public value is int but float in the shader to stay in floats for all the transform computations.

        const Backend::TransformCamera& recomputeDerived(const Transform& xformView) const;
        TransformCamera getEyeCamera(int eye, const StereoState& stereo, const Transform& xformView) const;
    };


    template<typename T, typename U>
    static void setGPUObject(const U& object, T* gpuObject) {
        object.gpuObject.setGPUObject(gpuObject);
    }
    template<typename T, typename U>
    static T* getGPUObject(const U& object) {
        return reinterpret_cast<T*>(object.gpuObject.getGPUObject());
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

template<typename F>
void doInBatch(std::shared_ptr<gpu::Context> context, F f) {
    static gpu::Batch::CacheState cacheState;
    gpu::Batch batch(cacheState);
    f(batch);
    context->render(batch);
    cacheState = batch.getCacheState();
}

};


#endif
