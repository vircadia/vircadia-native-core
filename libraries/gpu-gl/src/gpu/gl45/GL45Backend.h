//
//  GL45Backend.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_gpu_45_GL45Backend_h
#define hifi_gpu_45_GL45Backend_h

#include "../gl/GLBackend.h"
#include "../gl/GLTexture.h"
#include <thread>

#define INCREMENTAL_TRANSFER 0
#define THREADED_TEXTURE_BUFFERING 1

namespace gpu { namespace gl45 {
    
using namespace gpu::gl;
using TextureWeakPointer = std::weak_ptr<Texture>;

class GL45Backend : public GLBackend {
    using Parent = GLBackend;
    // Context Backend static interface required
    friend class Context;

public:
    explicit GL45Backend(bool syncCache) : Parent(syncCache) {}
    GL45Backend() : Parent() {}

    class GL45Texture : public GLTexture {
        using Parent = GLTexture;
        friend class GL45Backend;
        static GLuint allocate(const Texture& texture);
    protected:
        GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture);
        void generateMips() const override;
        void copyMipFaceFromTexture(uint16_t sourceMip, uint16_t targetMip, uint8_t face) const;
        void copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum format, GLenum type, const void* sourcePointer) const;
        virtual void syncSampler() const;
    };

    //
    // Textures that have fixed allocation sizes and cannot be managed at runtime
    //

    class GL45FixedAllocationTexture : public GL45Texture {
        using Parent = GL45Texture;
        friend class GL45Backend;

    public:
        GL45FixedAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture);
        ~GL45FixedAllocationTexture();

    protected:
        uint32 size() const override { return _size; }
        void allocateStorage() const;
        void syncSampler() const override;
        const uint32 _size { 0 };
    };

    class GL45AttachmentTexture : public GL45FixedAllocationTexture {
        using Parent = GL45FixedAllocationTexture;
        friend class GL45Backend;
    protected:
        GL45AttachmentTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture);
        ~GL45AttachmentTexture();
    };

    class GL45StrictResourceTexture : public GL45FixedAllocationTexture {
        using Parent = GL45FixedAllocationTexture;
        friend class GL45Backend;
    protected:
        GL45StrictResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture);
    };

    //
    // Textures that can be managed at runtime to increase or decrease their memory load
    //

    class GL45VariableAllocationTexture : public GL45Texture {
        using Parent = GL45Texture;
        friend class GL45Backend;
        using PromoteLambda = std::function<void()>;

    public:
        enum class MemoryPressureState {
            Idle,
            Transfer,
            Oversubscribed,
            Undersubscribed,
        };

        using QueuePair = std::pair<TextureWeakPointer, float>;
        struct QueuePairLess {
            bool operator()(const QueuePair& a, const QueuePair& b) {
                return a.second < b.second;
            }
        };
        using WorkQueue = std::priority_queue<QueuePair, std::vector<QueuePair>, QueuePairLess>;

        class TransferJob {
            using VoidLambda = std::function<void()>;
            using VoidLambdaQueue = std::queue<VoidLambda>;
            using ThreadPointer = std::shared_ptr<std::thread>;
            const GL45VariableAllocationTexture& _parent;
            // Holds the contents to transfer to the GPU in CPU memory
            std::vector<uint8_t> _buffer;
            // Indicates if a transfer from backing storage to interal storage has started
            bool _bufferingStarted { false };
            bool _bufferingCompleted { false };
            VoidLambda _transferLambda;
            VoidLambda _bufferingLambda;
#if THREADED_TEXTURE_BUFFERING
            static Mutex _mutex;
            static VoidLambdaQueue _bufferLambdaQueue;
            static ThreadPointer _bufferThread;
            static std::atomic<bool> _shutdownBufferingThread;
            static void bufferLoop();
#endif

        public:
            TransferJob(const TransferJob& other) = delete;
            TransferJob(const GL45VariableAllocationTexture& parent, std::function<void()> transferLambda);
            TransferJob(const GL45VariableAllocationTexture& parent, uint16_t sourceMip, uint16_t targetMip, uint8_t face, uint32_t lines = 0, uint32_t lineOffset = 0);
            ~TransferJob();
            bool tryTransfer();

#if THREADED_TEXTURE_BUFFERING
            static void startTransferLoop();
            static void stopTransferLoop();
#endif

        private:
            size_t _transferSize { 0 };
#if THREADED_TEXTURE_BUFFERING
            void startBuffering();
#endif
            void transfer();
        };

        using TransferQueue = std::queue<std::unique_ptr<TransferJob>>;
        static MemoryPressureState _memoryPressureState;
    protected:
        static size_t _frameTexturesCreated;
        static std::atomic<bool> _memoryPressureStateStale;
        static std::list<TextureWeakPointer> _memoryManagedTextures;
        static WorkQueue _transferQueue;
        static WorkQueue _promoteQueue;
        static WorkQueue _demoteQueue;
        static TexturePointer _currentTransferTexture;
        static const uvec3 INITIAL_MIP_TRANSFER_DIMENSIONS;


        static void updateMemoryPressure();
        static void processWorkQueues();
        static void addMemoryManagedTexture(const TexturePointer& texturePointer);
        static void addToWorkQueue(const TexturePointer& texture);
        static WorkQueue& getActiveWorkQueue();

        static void manageMemory();

    protected:
        GL45VariableAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture);
        ~GL45VariableAllocationTexture();
        //bool canPromoteNoAllocate() const { return _allocatedMip < _populatedMip; }
        bool canPromote() const { return _allocatedMip > 0; }
        bool canDemote() const { return _allocatedMip < _maxAllocatedMip; }
        bool hasPendingTransfers() const { return _populatedMip > _allocatedMip; }
        void executeNextTransfer(const TexturePointer& currentTexture);
        uint32 size() const override { return _size; }
        virtual void populateTransferQueue() = 0;
        virtual void promote() = 0;
        virtual void demote() = 0;

        // The allocated mip level, relative to the number of mips in the gpu::Texture object 
        // The relationship between a given glMip to the original gpu::Texture mip is always 
        // glMip + _allocatedMip
        uint16 _allocatedMip { 0 };
        // The populated mip level, relative to the number of mips in the gpu::Texture object
        // This must always be >= the allocated mip
        uint16 _populatedMip { 0 };
        // The highest (lowest resolution) mip that we will support, relative to the number 
        // of mips in the gpu::Texture object
        uint16 _maxAllocatedMip { 0 };
        uint32 _size { 0 };
        // Contains a series of lambdas that when executed will transfer data to the GPU, modify 
        // the _populatedMip and update the sampler in order to fully populate the allocated texture 
        // until _populatedMip == _allocatedMip
        TransferQueue _pendingTransfers;
    };

    class GL45ResourceTexture : public GL45VariableAllocationTexture {
        using Parent = GL45VariableAllocationTexture;
        friend class GL45Backend;
    protected:
        GL45ResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture);

        void syncSampler() const override;
        void promote() override;
        void demote() override;
        void populateTransferQueue() override;

        void allocateStorage(uint16 mip);
        void copyMipsFromTexture();
    };

#if 0
    class GL45SparseResourceTexture : public GL45VariableAllocationTexture {
        using Parent = GL45VariableAllocationTexture;
        friend class GL45Backend;
        using TextureTypeFormat = std::pair<GLenum, GLenum>;
        using PageDimensions = std::vector<uvec3>;
        using PageDimensionsMap = std::map<TextureTypeFormat, PageDimensions>;
        static PageDimensionsMap pageDimensionsByFormat;
        static Mutex pageDimensionsMutex;

        static bool isSparseEligible(const Texture& texture);
        static PageDimensions getPageDimensionsForFormat(const TextureTypeFormat& typeFormat);
        static PageDimensions getPageDimensionsForFormat(GLenum type, GLenum format);
        static const uint32_t DEFAULT_PAGE_DIMENSION = 128;
        static const uint32_t DEFAULT_MAX_SPARSE_LEVEL = 0xFFFF;

    protected:
        GL45SparseResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture);
        ~GL45SparseResourceTexture();
        uint32 size() const override { return _allocatedPages * _pageBytes; }
        void promote() override;
        void demote() override;

    private:
        uvec3 getPageCounts(const uvec3& dimensions) const;
        uint32_t getPageCount(const uvec3& dimensions) const;

        uint32_t _allocatedPages { 0 };
        uint32_t _pageBytes { 0 };
        uvec3 _pageDimensions { DEFAULT_PAGE_DIMENSION };
        GLuint _maxSparseLevel { DEFAULT_MAX_SPARSE_LEVEL };
    };
#endif


protected:

    void recycle() const override;

    GLuint getFramebufferID(const FramebufferPointer& framebuffer) override;
    GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer) override;

    GLuint getBufferID(const Buffer& buffer) override;
    GLBuffer* syncGPUObject(const Buffer& buffer) override;

    GLTexture* syncGPUObject(const TexturePointer& texture) override;

    GLuint getQueryID(const QueryPointer& query) override;
    GLQuery* syncGPUObject(const Query& query) override;

    // Draw Stage
    void do_draw(const Batch& batch, size_t paramOffset) override;
    void do_drawIndexed(const Batch& batch, size_t paramOffset) override;
    void do_drawInstanced(const Batch& batch, size_t paramOffset) override;
    void do_drawIndexedInstanced(const Batch& batch, size_t paramOffset) override;
    void do_multiDrawIndirect(const Batch& batch, size_t paramOffset) override;
    void do_multiDrawIndexedIndirect(const Batch& batch, size_t paramOffset) override;

    // Input Stage
    void resetInputStage() override;
    void updateInput() override;

    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void transferTransformState(const Batch& batch) const override;
    void initTransform() override;
    void updateTransform(const Batch& batch) override;

    // Output stage
    void do_blit(const Batch& batch, size_t paramOffset) override;

    // Texture Management Stage
    void initTextureManagementStage() override;
};

} }

Q_DECLARE_LOGGING_CATEGORY(gpugl45logging)

#endif

