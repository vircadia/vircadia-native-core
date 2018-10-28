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
#include <queue>

#include <GLMHelpers.h>

#include "Forward.h"
#include "Batch.h"
#include "Buffer.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Framebuffer.h"
#include "Frame.h"

class QImage;

namespace gpu {

//
// GL Backend pointer storage mechanism
// One of the following three defines must be defined.
// GPU_POINTER_STORAGE_SHARED

// The platonic ideal, use references to smart pointers.
// However, this produces artifacts because there are too many places in the code right now that
// create temporary values (undesirable smart pointer duplications) and then those temp variables
// get passed on and have their reference taken, and then invalidated
// GPU_POINTER_STORAGE_REF

// Raw pointer manipulation.  Seems more dangerous than the reference wrappers,
// but in practice, the danger of grabbing a reference to a temporary variable
// is causing issues
// GPU_POINTER_STORAGE_RAW

#if defined(USE_GLES)
#define GPU_POINTER_STORAGE_SHARED
#else
#define GPU_POINTER_STORAGE_RAW
#endif

#if defined(GPU_POINTER_STORAGE_SHARED)
template <typename T>
static inline bool compare(const std::shared_ptr<T>& a, const std::shared_ptr<T>& b) {
    return a == b;
}

template <typename T>
static inline T* acquire(const std::shared_ptr<T>& pointer) {
    return pointer.get();
}

template <typename T>
static inline void reset(std::shared_ptr<T>& pointer) {
    return pointer.reset();
}

template <typename T>
static inline bool valid(const std::shared_ptr<T>& pointer) {
    return pointer.operator bool();
}

template <typename T>
static inline void assign(std::shared_ptr<T>& pointer, const std::shared_ptr<T>& source) {
    pointer = source;
}

using BufferReference = BufferPointer;
using TextureReference = TexturePointer;
using FramebufferReference = FramebufferPointer;
using FormatReference = Stream::FormatPointer;
using PipelineReference = PipelinePointer;

#define GPU_REFERENCE_INIT_VALUE nullptr

#elif defined(GPU_POINTER_STORAGE_REF)

template <typename T>
class PointerReferenceWrapper : public std::reference_wrapper<const std::shared_ptr<T>> {
    using Parent = std::reference_wrapper<const std::shared_ptr<T>>;

public:
    using Pointer = std::shared_ptr<T>;
    PointerReferenceWrapper() : Parent(EMPTY()) {}
    PointerReferenceWrapper(const Pointer& pointer) : Parent(pointer) {}
    void clear() { *this = EMPTY(); }

private:
    static const Pointer& EMPTY() {
        static const Pointer EMPTY_VALUE;
        return EMPTY_VALUE;
    };
};

template <typename T>
static bool compare(const PointerReferenceWrapper<T>& reference, const std::shared_ptr<T>& pointer) {
    return reference.get() == pointer;
}

template <typename T>
static inline T* acquire(const PointerReferenceWrapper<T>& reference) {
    return reference.get().get();
}

template <typename T>
static void assign(PointerReferenceWrapper<T>& reference, const std::shared_ptr<T>& pointer) {
    reference = pointer;
}

template <typename T>
static bool valid(const PointerReferenceWrapper<T>& reference) {
    return reference.get().operator bool();
}

template <typename T>
static inline void reset(PointerReferenceWrapper<T>& reference) {
    return reference.clear();
}

using BufferReference = PointerReferenceWrapper<Buffer>;
using TextureReference = PointerReferenceWrapper<Texture>;
using FramebufferReference = PointerReferenceWrapper<Framebuffer>;
using FormatReference = PointerReferenceWrapper<Stream::Format>;
using PipelineReference = PointerReferenceWrapper<Pipeline>;

#define GPU_REFERENCE_INIT_VALUE

#elif defined(GPU_POINTER_STORAGE_RAW)

template <typename T>
static bool compare(const T* const& rawPointer, const std::shared_ptr<T>& pointer) {
    return rawPointer == pointer.get();
}

template <typename T>
static inline T* acquire(T* const& rawPointer) {
    return rawPointer;
}

template <typename T>
static inline bool valid(const T* const& rawPointer) {
    return rawPointer;
}

template <typename T>
static inline void reset(T*& rawPointer) {
    rawPointer = nullptr;
}

template <typename T>
static inline void assign(T*& rawPointer, const std::shared_ptr<T>& pointer) {
    rawPointer = pointer.get();
}

using BufferReference = Buffer*;
using TextureReference = Texture*;
using FramebufferReference = Framebuffer*;
using FormatReference = Stream::Format*;
using PipelineReference = Pipeline*;

#define GPU_REFERENCE_INIT_VALUE nullptr

#endif


struct ContextStats {
public:
    int _ISNumFormatChanges = 0;
    int _ISNumInputBufferChanges = 0;
    int _ISNumIndexBufferChanges = 0;

    int _RSNumResourceBufferBounded = 0;
    int _RSNumTextureBounded = 0;
    int _RSAmountTextureMemoryBounded = 0;

    int _DSNumAPIDrawcalls = 0;
    int _DSNumDrawcalls = 0;
    int _DSNumTriangles = 0;

    int _PSNumSetPipelines = 0;

    ContextStats() {}
    ContextStats(const ContextStats& stats) = default;

    void evalDelta(const ContextStats& begin, const ContextStats& end);
};

class Backend {
public:
    virtual ~Backend(){};

    virtual void shutdown() {}
    virtual const std::string& getVersion() const = 0;

    void setStereoState(const StereoState& stereo) { _stereo = stereo; }

    virtual void render(const Batch& batch) = 0;
    virtual void syncCache() = 0;
    virtual void syncProgram(const gpu::ShaderPointer& program) = 0;
    virtual void recycle() const = 0;
    virtual void downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) = 0;

    virtual bool supportedTextureFormat(const gpu::Element& format) = 0;

    // Shared header between C++ and GLSL
#include "TransformCamera_shared.slh"

    class TransformCamera : public _TransformCamera {
    public:
        const Backend::TransformCamera& recomputeDerived(const Transform& xformView) const;
        // Jitter should be divided by framebuffer size
        TransformCamera getMonoCamera(const Transform& xformView, Vec2 normalizedJitter) const;
        // Jitter should be divided by framebuffer size
        TransformCamera getEyeCamera(int eye, const StereoState& stereo, const Transform& xformView, Vec2 normalizedJitter) const;
    };

    template <typename T, typename U>
    static void setGPUObject(const U& object, T* gpuObject) {
        object.gpuObject.setGPUObject(gpuObject);
    }
    template <typename T, typename U>
    static T* getGPUObject(const U& object) {
        return reinterpret_cast<T*>(object.gpuObject.getGPUObject());
    }

    void resetStats() const { _stats = ContextStats(); }
    void getStats(ContextStats& stats) const { stats = _stats; }

    virtual bool isTextureManagementSparseEnabled() const = 0;

    // These should only be accessed by Backend implementation to report the buffer and texture allocations,
    // they are NOT public objects
    static ContextMetricSize freeGPUMemSize;

    static ContextMetricCount bufferCount;
    static ContextMetricSize bufferGPUMemSize;

    static ContextMetricCount textureResidentCount;
    static ContextMetricCount textureFramebufferCount;
    static ContextMetricCount textureResourceCount;
    static ContextMetricCount textureExternalCount;

    static ContextMetricSize textureResidentGPUMemSize;
    static ContextMetricSize textureFramebufferGPUMemSize;
    static ContextMetricSize textureResourceGPUMemSize;
    static ContextMetricSize textureExternalGPUMemSize;

    static ContextMetricCount texturePendingGPUTransferCount;
    static ContextMetricSize texturePendingGPUTransferMemSize;
    static ContextMetricSize textureResourcePopulatedGPUMemSize;
    static ContextMetricSize textureResourceIdealGPUMemSize;

protected:
    virtual bool isStereo() const {
        return _stereo.isStereo();
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
    mutable ContextStats _stats;
    StereoState _stereo;
};

class Context {
public:
    using Size = Resource::Size;
    typedef BackendPointer (*CreateBackend)();

    // This one call must happen before any context is created or used (Shader::MakeProgram) in order to setup the Backend and any singleton data needed
    template <class T>
    static void init() {
        std::call_once(_initialized, [] {
            _createBackendCallback = T::createBackend;
            T::init();
        });
    }

    Context();
    ~Context();

    void shutdown();
    const std::string& getBackendVersion() const;

    void beginFrame(const glm::mat4& renderView = glm::mat4(), const glm::mat4& renderPose = glm::mat4());
    void appendFrameBatch(const BatchPointer& batch);
    FramePointer endFrame();

    BatchPointer acquireBatch(const char* name = nullptr);
    void releaseBatch(Batch* batch);

    // MUST only be called on the rendering thread
    //
    // Handle any pending operations to clean up (recycle / deallocate) resources no longer in use
    void recycle() const;

    // MUST only be called on the rendering thread
    //
    // Execute a batch immediately, rather than as part of a frame
    void executeBatch(Batch& batch) const;

    // MUST only be called on the rendering thread
    //
    // Executes a frame, applying any updates contained in the frame batches to the rendering
    // thread shadow copies.  Either executeFrame or consumeFrameUpdates MUST be called on every frame
    // generated, IN THE ORDER they were generated.
    void executeFrame(const FramePointer& frame) const;

    // MUST only be called on the rendering thread.
    //
    // Consuming a frame applies any updates queued from the recording thread and applies them to the
    // shadow copy used by the rendering thread.
    //
    // EVERY frame generated MUST be consumed, regardless of whether the frame is actually executed,
    // or the buffer shadow copies can become unsynced from the recording thread copies.
    //
    // Consuming a frame is idempotent, as the frame encapsulates the updates and clears them out as
    // it applies them, so calling it more than once on a given frame will have no effect after the
    // first time
    //
    //
    // This is automatically called by executeFrame, so you only need to call it if you
    // have frames you aren't going to otherwise execute, for instance when a display plugin is
    // being disabled, or in the null display plugin where no rendering actually occurs
    void consumeFrameUpdates(const FramePointer& frame) const;

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
    void resetStats() const;
    void getStats(ContextStats& stats) const;

    // Same as above but grabbed at every end of a frame
    void getFrameStats(ContextStats& stats) const;

	static PipelinePointer createMipGenerationPipeline(const ShaderPointer& pixelShader);

    double getFrameTimerGPUAverage() const;
    double getFrameTimerBatchAverage() const;

    static Size getFreeGPUMemSize();
    static Size getUsedGPUMemSize();

    static uint32_t getBufferGPUCount();
    static Size getBufferGPUMemSize();

    static uint32_t getTextureGPUCount();
    static uint32_t getTextureResidentGPUCount();
    static uint32_t getTextureFramebufferGPUCount();
    static uint32_t getTextureResourceGPUCount();
    static uint32_t getTextureExternalGPUCount();

    static Size getTextureGPUMemSize();
    static Size getTextureResidentGPUMemSize();
    static Size getTextureFramebufferGPUMemSize();
    static Size getTextureResourceGPUMemSize();
    static Size getTextureExternalGPUMemSize();

    static uint32_t getTexturePendingGPUTransferCount();
    static Size getTexturePendingGPUTransferMemSize();

    static Size getTextureResourcePopulatedGPUMemSize();
    static Size getTextureResourceIdealGPUMemSize();

    struct ProgramsToSync {
        ProgramsToSync(const std::vector<gpu::ShaderPointer>& programs, std::function<void()> callback, size_t rate) :
            programs(programs), callback(callback), rate(rate) {}

        std::vector<gpu::ShaderPointer> programs;
        std::function<void()> callback;
        size_t rate;
    };

    void pushProgramsToSync(const std::vector<uint32_t>& programIDs, std::function<void()> callback, size_t rate = 0);
    void pushProgramsToSync(const std::vector<gpu::ShaderPointer>& programs, std::function<void()> callback, size_t rate = 0);

    void processProgramsToSync();

protected:
    Context(const Context& context);

    std::shared_ptr<Backend> _backend;
    std::mutex _batchPoolMutex;
    std::list<Batch*> _batchPool;
    bool _frameActive{ false };
    FramePointer _currentFrame;
    RangeTimerPointer _frameRangeTimer;
    StereoState _stereo;

    std::mutex _programsToSyncMutex;
    std::queue<ProgramsToSync> _programsToSyncQueue;
    gpu::Shaders _syncedPrograms;
    size_t _nextProgramToSyncIndex { 0 };

    // Sampled at the end of every frame, the stats of all the counters
    mutable ContextStats _frameStats;

    static CreateBackend _createBackendCallback;
    static std::once_flag _initialized;

    friend class Shader;
    friend class Backend;
};
typedef std::shared_ptr<Context> ContextPointer;

void doInBatch(const char* name, const std::shared_ptr<gpu::Context>& context, const std::function<void(Batch& batch)>& f);

};  // namespace gpu

#endif
