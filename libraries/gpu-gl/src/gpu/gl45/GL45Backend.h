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

namespace gpu { namespace gl45 {
    
using namespace gpu::gl;

struct TransferState {
    GLTexture& _texture;
    GLenum _internalFormat { GL_RGBA8 };
    GLTexelFormat _texelFormat;
    uint8_t _face { 0 };
    uint16_t  _mipLevel { 0 };
    uint32_t _bytesPerLine { 0 };
    uint32_t _bytesPerPixel { 0 };
    uint32_t _bytesPerPage { 0 };
    GLuint _maxSparseLevel { 0 };

    uvec3 _mipDimensions;
    uvec3 _mipOffset;
    uvec3 _pageSize;
    const uint8_t* _srcPointer { nullptr };
    uvec3 currentPageSize() const;
    void updateSparse();
    void updateMip();
    void populatePage(uint8_t* dest);
    bool increment();
    TransferState(GLTexture& texture);
};

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
    public:
        GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, bool transferrable);
        GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLTexture* original);
        ~GL45Texture();

    protected:
        void startTransfer() override;
        bool continueTransfer() override;
        void incrementalTransfer(const uvec3& size, const gpu::Texture::PixelsPointer& mip, std::function<void(const ivec3& offset, const uvec3& size)> f) const;
        void transferMip(uint16_t mipLevel, uint8_t face = 0) const;
        void allocateMip(uint16_t mipLevel, uint8_t face = 0) const;
        void allocateStorage() const override;
        void updateSize() const override;
        void syncSampler() const override;
        void generateMips() const override;
        void withPreservedTexture(std::function<void()> f) const override;

        TransferState _transferState;
    };


protected:
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
};

} }

Q_DECLARE_LOGGING_CATEGORY(gpugl45logging)


#endif
