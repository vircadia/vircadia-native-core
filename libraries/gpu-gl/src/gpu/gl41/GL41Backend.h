//
//  GL41Backend.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/27/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_41_GL41Backend_h
#define hifi_gpu_41_GL41Backend_h

#include <gl/Config.h>

#include "../gl/GLBackend.h"
#include "../gl/GLTexture.h"

#define GPU_CORE_41 410
#define GPU_CORE_43 430

#ifdef Q_OS_MAC
#define GPU_INPUT_PROFILE GPU_CORE_41
#else 
#define GPU_INPUT_PROFILE GPU_CORE_43
#endif

namespace gpu { namespace gl41 {

using namespace gpu::gl;

class GL41Backend : public GLBackend {
    using Parent = GLBackend;
    // Context Backend static interface required
    friend class Context;

public:
    explicit GL41Backend(bool syncCache) : Parent(syncCache) {}
    GL41Backend() : Parent() {}

    class GL41Texture : public GLTexture {
        using Parent = GLTexture;
        GLuint allocate();
    public:
        GL41Texture(const std::weak_ptr<GLBackend>& backend, const Texture& buffer, bool transferrable);
        GL41Texture(const std::weak_ptr<GLBackend>& backend, const Texture& buffer, GL41Texture* original);

    protected:
        void transferMip(uint16_t mipLevel, uint8_t face) const;
        void startTransfer() override;
        void allocateStorage() const override;
        void updateSize() const override;
        void syncSampler() const override;
        void generateMips() const override;
        void withPreservedTexture(std::function<void()> f) const override;
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

Q_DECLARE_LOGGING_CATEGORY(gpugl41logging)


#endif
