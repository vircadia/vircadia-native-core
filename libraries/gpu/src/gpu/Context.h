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

#include "Forward.h"
#include "Batch.h"
#include "Resource.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "Frame.h"

class QImage;

namespace gpu {

struct ContextStats {
public:
    int _ISNumFormatChanges = 0;
    int _ISNumInputBufferChanges = 0;
    int _ISNumIndexBufferChanges = 0;

    int _RSNumTextureBounded = 0;
    int _RSAmountTextureMemoryBounded = 0;

    int _DSNumAPIDrawcalls = 0;
    int _DSNumDrawcalls = 0;
    int _DSNumTriangles = 0;

    int _PSNumSetPipelines = 0;
 
    ContextStats() {}
    ContextStats(const ContextStats& stats) = default;
};

class Backend {
public:
    virtual~ Backend() {};

    void setStereoState(const StereoState& stereo) { _stereo = stereo; }

    virtual void render(Batch& batch) = 0;
    virtual void syncCache() = 0;
    virtual void cleanupTrash() const = 0;
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) = 0;

    // UBO class... layout MUST match the layout in Transform.slh
    class TransformCamera {
    public:
        mutable Mat4 _view;
        mutable Mat4 _viewInverse;
        mutable Mat4 _projectionViewUntranslated;
        Mat4 _projection;
        mutable Mat4 _projectionInverse;
        Vec4 _viewport; // Public value is int but float in the shader to stay in floats for all the transform computations.
        mutable Vec4 _stereoInfo;

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

    void getStats(ContextStats& stats) const { stats = _stats; }



    // These should only be accessed by Backend implementation to repport the buffer and texture allocations,
    // they are NOT public calls
    static void incrementBufferGPUCount();
    static void decrementBufferGPUCount();
    static void updateBufferGPUMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize);
    static void incrementTextureGPUCount();
    static void decrementTextureGPUCount();
    static void updateTextureGPUMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize);
    static void updateTextureGPUVirtualMemoryUsage(Resource::Size prevObjectSize, Resource::Size newObjectSize);
    static void incrementTextureGPUTransferCount();
    static void decrementTextureGPUTransferCount();

protected:
    virtual bool isStereo() {
        return _stereo._enable;
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

    friend class Context;
    ContextStats _stats;
    StereoState _stereo;
};

class Context {
public:
    using Size = Resource::Size;
    typedef BackendPointer (*CreateBackend)();
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

    void beginFrame(const glm::mat4& renderPose = glm::mat4());
    void append(Batch& batch);
    FramePointer endFrame();

    const BackendPointer& getBackend() const { return _backend; }

    void enableStereo(bool enable = true);
    bool isStereo();
    void setStereoProjections(const mat4 eyeProjections[2]);
    void setStereoViews(const mat4 eyeViews[2]);
    void getStereoProjections(mat4* eyeProjections) const;
    void getStereoViews(mat4* eyeViews) const;

    // Downloading the Framebuffer is a synchronous action that is not efficient.
    // It s here for convenience to easily capture a snapshot
    void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage);

     // Repporting stats of the context
    void getStats(ContextStats& stats) const;


    static uint32_t getBufferGPUCount();
    static Size getBufferGPUMemoryUsage();

    static uint32_t getTextureGPUCount();
    static Size getTextureGPUMemoryUsage();
    static Size getTextureGPUVirtualMemoryUsage();
    static uint32_t getTextureGPUTransferCount();

protected:
    Context(const Context& context);

    std::shared_ptr<Backend> _backend;
    bool _frameActive { false };
    FramePointer _currentFrame;
    StereoState  _stereo;

    // This function can only be called by "static Shader::makeProgram()"
    // makeProgramShader(...) make a program shader ready to be used in a Batch.
    // It compiles the sub shaders, link them and defines the Slots and their bindings.
    // If the shader passed is not a program, nothing happens. 
    static bool makeProgram(Shader& shader, const Shader::BindingSet& bindings);

    static CreateBackend _createBackendCallback;
    static MakeProgram _makeProgramCallback;
    static std::once_flag _initialized;

    friend class Shader;

    // These should only be accessed by the Backend, they are NOT public calls
    static void incrementBufferGPUCount();
    static void decrementBufferGPUCount();
    static void updateBufferGPUMemoryUsage(Size prevObjectSize, Size newObjectSize);

    static void incrementTextureGPUCount();
    static void decrementTextureGPUCount();
    static void updateTextureGPUMemoryUsage(Size prevObjectSize, Size newObjectSize);
    static void updateTextureGPUVirtualMemoryUsage(Size prevObjectSize, Size newObjectSize);
    static void incrementTextureGPUTransferCount();
    static void decrementTextureGPUTransferCount();

    // Buffer and Texture Counters
    static std::atomic<uint32_t> _bufferGPUCount;
    static std::atomic<Size> _bufferGPUMemoryUsage;

    static std::atomic<uint32_t> _textureGPUCount;
    static std::atomic<Size> _textureGPUMemoryUsage;
    static std::atomic<Size> _textureGPUVirtualMemoryUsage;
    static std::atomic<uint32_t> _textureGPUTransferCount;


    friend class Backend;
};
typedef std::shared_ptr<Context> ContextPointer;

template<typename F>
void doInBatch(std::shared_ptr<gpu::Context> context, F f) {
    gpu::Batch batch;
    f(batch);
    context->append(batch);
}

};


#endif
