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
#ifndef hifi_gpu_45_GL45Backend_h
#define hifi_gpu_45_GL45Backend_h

#include "../gl/GLBackend.h"
#include "../gl/GLTexture.h"

#define INCREMENTAL_TRANSFER 0

namespace gpu { namespace gl45 {
    
using namespace gpu::gl;

class GL45Backend : public GLBackend {
    using Parent = GLBackend;
    // Context Backend static interface required
    friend class Context;

public:
    explicit GL45Backend(bool syncCache) : Parent(syncCache) {}
    GL45Backend() : Parent() {}

    class GL45Texture : public GLTexture {
        using Parent = GLTexture;
        static GLuint allocate(const Texture& texture);
        static const uint32_t DEFAULT_PAGE_DIMENSION = 128;
        static const uint32_t DEFAULT_MAX_SPARSE_LEVEL = 0xFFFF;

    public:
        GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint externalId);
        GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, bool transferrable);
        ~GL45Texture();

        void postTransfer() override;

        struct SparseInfo {
            SparseInfo(GL45Texture& texture);
            void maybeMakeSparse();
            void update();
            uvec3 getPageCounts(const uvec3& dimensions) const;
            uint32_t getPageCount(const uvec3& dimensions) const;

            GL45Texture& texture;
            bool sparse { false };
            uvec3 pageDimensions { DEFAULT_PAGE_DIMENSION };
            GLuint maxSparseLevel { DEFAULT_MAX_SPARSE_LEVEL };
            uint32_t maxPages { 0 };
            uint32_t pageBytes { 0 };
            GLint pageDimensionsIndex { 0 };
        };

#if INCREMENTAL_TRANSFER
        struct TransferState {
            TransferState(GL45Texture& texture);
            uvec3 currentPageSize() const;
            void updateMip();
            void populatePage(std::vector<uint8_t>& dest);
            bool increment();

            GL45Texture& texture;
            GLTexelFormat texelFormat;
            uint8_t face { 0 };
            uint16_t mipLevel { 0 };
            uint32_t bytesPerLine { 0 };
            uint32_t bytesPerPixel { 0 };
            uint32_t bytesPerPage { 0 };
            uvec3 mipDimensions;
            uvec3 mipOffset;
            const uint8_t* srcPointer { nullptr };
        };
    protected:
        TransferState _transferState;
#endif

    protected:
        void updateMips() override;
        void stripToMip(uint16_t newMinMip);
        void startTransfer() override;
        bool continueTransfer() override;
        void finishTransfer() override;
        void incrementalTransfer(const uvec3& size, const gpu::Texture::PixelsPointer& mip, std::function<void(const ivec3& offset, const uvec3& size)> f) const;
        void transferMip(uint16_t mipLevel, uint8_t face = 0) const;
        void allocateMip(uint16_t mipLevel, uint8_t face = 0) const;
        void allocateStorage() const override;
        void updateSize() const override;
        void syncSampler() const override;
        void generateMips() const override;
        void withPreservedTexture(std::function<void()> f) const override;
        void derez();

        SparseInfo _sparseInfo;
        uint32_t _allocatedPages { 0 };
        uint32_t _lastMipAllocatedPages { 0 };
        uint16_t _mipOffset { 0 };
        friend class GL45Backend;
    };


protected:
    void recycle() const override;
    void derezTextures() const;

    GLuint getFramebufferID(const FramebufferPointer& framebuffer) override;
    GLFramebuffer* syncGPUObject(const Framebuffer& framebuffer) override;

    GLuint getBufferID(const Buffer& buffer) override;
    GLBuffer* syncGPUObject(const Buffer& buffer) override;

    GLuint getTextureID(const TexturePointer& texture, bool needTransfer = true) override;
    GLTexture* syncGPUObject(const TexturePointer& texture, bool sync = true) override;

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
    void updateInput() override;

    // Synchronize the state cache of this Backend with the actual real state of the GL Context
    void transferTransformState(const Batch& batch) const override;
    void initTransform() override;
    void updateTransform(const Batch& batch);
    void resetTransformStage();

    // Output stage
    void do_blit(const Batch& batch, size_t paramOffset) override;

    // Texture Management Stage
    void initTextureManagementStage() override;
};

} }

Q_DECLARE_LOGGING_CATEGORY(gpugl45logging)


#endif
