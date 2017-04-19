//
//  GL41BackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"

#include <unordered_set>
#include <unordered_map>

#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl41;

GLTexture* GL41Backend::syncGPUObject(const TexturePointer& texturePointer) {
    if (!texturePointer) {
        return nullptr;
    }

    const Texture& texture = *texturePointer;
    if (TextureUsageType::EXTERNAL == texture.getUsageType()) {
        return Parent::syncGPUObject(texturePointer);
    }

    if (!texture.isDefined()) {
        // NO texture definition yet so let's avoid thinking
        return nullptr;
    }

    GL41Texture* object = Backend::getGPUObject<GL41Texture>(texture);
    if (!object) {
        switch (texture.getUsageType()) {
            case TextureUsageType::RENDERBUFFER:
                object = new GL41AttachmentTexture(shared_from_this(), texture);
                break;

            case TextureUsageType::STRICT_RESOURCE:
                qCDebug(gpugllogging) << "Strict texture " << texture.source().c_str();
                object = new GL41StrictResourceTexture(shared_from_this(), texture);
                break;

            case TextureUsageType::RESOURCE:
                qCDebug(gpugllogging) << "variable / Strict texture " << texture.source().c_str();
                object = new GL41ResourceTexture(shared_from_this(), texture);
                GLVariableAllocationSupport::addMemoryManagedTexture(texturePointer);
                break;

            default:
                Q_UNREACHABLE();
        }
    }

    return object;
}

using GL41Texture = GL41Backend::GL41Texture;

GL41Texture::GL41Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture)
    : GLTexture(backend, texture, allocate(texture)) {
    incrementTextureGPUCount();
}

GLuint GL41Texture::allocate(const Texture& texture) {
    GLuint result;
    // FIXME technically GL 4.2, but OSX includes the ARB_texture_storage extension
    glCreateTextures(getGLTextureType(texture), 1, &result);
    //glGenTextures(1, &result);
    return result;
}


void GL41Texture::withPreservedTexture(std::function<void()> f) const {
    const GLint TRANSFER_TEXTURE_UNIT = 32;
    glActiveTexture(GL_TEXTURE0 + TRANSFER_TEXTURE_UNIT);
    glBindTexture(_target, _texture);
    (void)CHECK_GL_ERROR();

    f();
    glBindTexture(_target, 0);
    (void)CHECK_GL_ERROR();
}


void GL41Texture::generateMips() const {
    withPreservedTexture([&] {
        glGenerateMipmap(_target);
    });
    (void)CHECK_GL_ERROR();
}

void GL41Texture::copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum format, GLenum type, const void* sourcePointer) const {
    if (GL_TEXTURE_2D == _target) {
        glTexSubImage2D(_target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
    } else if (GL_TEXTURE_CUBE_MAP == _target) {
        auto target = GLTexture::CUBE_FACE_LAYOUT[face];
        glTexSubImage2D(target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
    } else {
        assert(false);
    }
    (void)CHECK_GL_ERROR();
}

void GL41Texture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();

    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    if (sampler.doComparison()) {
        glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(_target, GL_TEXTURE_COMPARE_FUNC, COMPARISON_TO_GL[sampler.getComparisonFunction()]);
    } else {
        glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    glTexParameteri(_target, GL_TEXTURE_WRAP_S, WRAP_MODES[sampler.getWrapModeU()]);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, WRAP_MODES[sampler.getWrapModeV()]);
    glTexParameteri(_target, GL_TEXTURE_WRAP_R, WRAP_MODES[sampler.getWrapModeW()]);

    glTexParameterfv(_target, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, (uint16)sampler.getMipOffset());

    glTexParameterf(_target, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTexParameterf(_target, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));

    glTexParameterf(_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
}

using GL41FixedAllocationTexture = GL41Backend::GL41FixedAllocationTexture;

GL41FixedAllocationTexture::GL41FixedAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41Texture(backend, texture), _size(texture.evalTotalSize()) {
    withPreservedTexture([&] {
        allocateStorage();
        syncSampler();
    });
}

GL41FixedAllocationTexture::~GL41FixedAllocationTexture() {
}

void GL41FixedAllocationTexture::allocateStorage() const {
    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const auto numMips = _gpuObject.getNumMips();

    // glTextureStorage2D(_id, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);
    for (GLint level = 0; level < numMips; level++) {
        Vec3u dimensions = _gpuObject.evalMipDimensions(level);
        for (GLenum target : getFaceTargets(_target)) {
            glTexImage2D(target, level, texelFormat.internalFormat, dimensions.x, dimensions.y, 0, texelFormat.format, texelFormat.type, nullptr);
        }
    }

    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(_target, GL_TEXTURE_MAX_LEVEL, numMips - 1);
}

void GL41FixedAllocationTexture::syncSampler() const {
    Parent::syncSampler();
    const Sampler& sampler = _gpuObject.getSampler();
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), sampler.getMinMip());

    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTexParameterf(_target, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTexParameterf(_target, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.0f : sampler.getMaxMip()));
}

// Renderbuffer attachment textures
using GL41AttachmentTexture = GL41Backend::GL41AttachmentTexture;

GL41AttachmentTexture::GL41AttachmentTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41FixedAllocationTexture(backend, texture) {
    Backend::updateTextureGPUFramebufferMemoryUsage(0, size());
}

GL41AttachmentTexture::~GL41AttachmentTexture() {
    Backend::updateTextureGPUFramebufferMemoryUsage(size(), 0);
}

// Strict resource textures
using GL41StrictResourceTexture = GL41Backend::GL41StrictResourceTexture;

GL41StrictResourceTexture::GL41StrictResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41FixedAllocationTexture(backend, texture) {
    withPreservedTexture([&] {
   
        auto mipLevels = _gpuObject.getNumMips();
        for (uint16_t sourceMip = 0; sourceMip < mipLevels; sourceMip++) {
            uint16_t targetMip = sourceMip;
            size_t maxFace = GLTexture::getFaceCount(_target);
            for (uint8_t face = 0; face < maxFace; face++) {
                copyMipFaceFromTexture(sourceMip, targetMip, face);
            }
        }
    });

    if (texture.isAutogenerateMips()) {
        generateMips();
    }
}

using GL41VariableAllocationTexture = GL41Backend::GL41VariableAllocationTexture;

GL41VariableAllocationTexture::GL41VariableAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41Texture(backend, texture) {
    auto mipLevels = texture.getNumMips();
    _allocatedMip = mipLevels;
    uvec3 mipDimensions;
    for (uint16_t mip = 0; mip < mipLevels; ++mip) {
        if (glm::all(glm::lessThanEqual(texture.evalMipDimensions(mip), INITIAL_MIP_TRANSFER_DIMENSIONS))) {
            _maxAllocatedMip = _populatedMip = mip;
            break;
        }
    }

    uint16_t allocatedMip = _populatedMip - std::min<uint16_t>(_populatedMip, 2);
    allocateStorage(allocatedMip);
    _memoryPressureStateStale = true;
    size_t maxFace = GLTexture::getFaceCount(_target);
    for (uint16_t sourceMip = _populatedMip; sourceMip < mipLevels; ++sourceMip) {
        uint16_t targetMip = sourceMip - _allocatedMip;
        for (uint8_t face = 0; face < maxFace; ++face) {
            copyMipFaceFromTexture(sourceMip, targetMip, face);
        }
    }
    syncSampler();
}

GL41VariableAllocationTexture::~GL41VariableAllocationTexture() {
    Backend::updateTextureGPUMemoryUsage(_size, 0);
}

void GL41VariableAllocationTexture::allocateStorage(uint16 allocatedMip) {
    _allocatedMip = allocatedMip;

    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const auto dimensions = _gpuObject.evalMipDimensions(_allocatedMip);
    const auto totalMips = _gpuObject.getNumMips();
    const auto mips = totalMips - _allocatedMip;
    withPreservedTexture([&] {
        // FIXME technically GL 4.2, but OSX includes the ARB_texture_storage extension
        glTexStorage2D(_target, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);
    });
    auto mipLevels = _gpuObject.getNumMips();
    _size = 0;
    for (uint16_t mip = _allocatedMip; mip < mipLevels; ++mip) {
        _size += _gpuObject.evalMipSize(mip);
    }
    Backend::updateTextureGPUMemoryUsage(0, _size);
}


void GL41VariableAllocationTexture::copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum format, GLenum type, const void* sourcePointer) const {
    withPreservedTexture([&] {
        Parent::copyMipFaceLinesFromTexture(mip, face, size, yOffset, format, type, sourcePointer);
    });
}

void GL41VariableAllocationTexture::syncSampler() const {
    withPreservedTexture([&] {
        Parent::syncSampler();
        glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, _populatedMip - _allocatedMip);
    });
}

void GL41VariableAllocationTexture::promote() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    Q_ASSERT(_allocatedMip > 0);
    GLuint oldId = _id;
    auto oldSize = _size;
    // create new texture
    const_cast<GLuint&>(_id) = allocate(_gpuObject);
    uint16_t oldAllocatedMip = _allocatedMip;

    // allocate storage for new level
    allocateStorage(_allocatedMip - std::min<uint16_t>(_allocatedMip, 2));

    withPreservedTexture([&] {
        GLuint fbo { 0 };
        glCreateFramebuffers(1, &fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

        uint16_t mips = _gpuObject.getNumMips();
        // copy pre-existing mips
        for (uint16_t mip = _populatedMip; mip < mips; ++mip) {
            auto mipDimensions = _gpuObject.evalMipDimensions(mip);
            uint16_t targetMip = mip - _allocatedMip;
            uint16_t sourceMip = mip - oldAllocatedMip;
            for (GLenum target : getFaceTargets(_target)) {
                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, oldId, sourceMip);
                (void)CHECK_GL_ERROR();
                glCopyTexSubImage2D(target, targetMip, 0, 0, 0, 0, mipDimensions.x, mipDimensions.y);
                (void)CHECK_GL_ERROR();
            }
        }

        // destroy the transfer framebuffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);

        syncSampler();
    });

    // destroy the old texture
    glDeleteTextures(1, &oldId);
    // update the memory usage
    Backend::updateTextureGPUMemoryUsage(oldSize, 0);
    populateTransferQueue();
}

void GL41VariableAllocationTexture::demote() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    Q_ASSERT(_allocatedMip < _maxAllocatedMip);
    auto oldId = _id;
    auto oldSize = _size;
    const_cast<GLuint&>(_id) = allocate(_gpuObject);
    uint16_t oldAllocatedMip = _allocatedMip;
    allocateStorage(_allocatedMip + 1);
    _populatedMip = std::max(_populatedMip, _allocatedMip);

    withPreservedTexture([&] {
        GLuint fbo { 0 };
        glCreateFramebuffers(1, &fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

        uint16_t mips = _gpuObject.getNumMips();
        // copy pre-existing mips
        for (uint16_t mip = _populatedMip; mip < mips; ++mip) {
            auto mipDimensions = _gpuObject.evalMipDimensions(mip);
            uint16_t targetMip = mip - _allocatedMip;
            uint16_t sourceMip = mip - oldAllocatedMip;
            for (GLenum target : getFaceTargets(_target)) {
                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, oldId, sourceMip);
                (void)CHECK_GL_ERROR();
                glCopyTexSubImage2D(target, targetMip, 0, 0, 0, 0, mipDimensions.x, mipDimensions.y);
                (void)CHECK_GL_ERROR();
            }
        }

        // destroy the transfer framebuffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);

        syncSampler();
    });


    // destroy the old texture
    glDeleteTextures(1, &oldId);
    // update the memory usage
    Backend::updateTextureGPUMemoryUsage(oldSize, 0);
    populateTransferQueue();
}


void GL41VariableAllocationTexture::populateTransferQueue() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    if (_populatedMip <= _allocatedMip) {
        return;
    }
    _pendingTransfers = TransferQueue();

    const uint8_t maxFace = GLTexture::getFaceCount(_target);
    uint16_t sourceMip = _populatedMip;
    do {
        --sourceMip;
        auto targetMip = sourceMip - _allocatedMip;
        auto mipDimensions = _gpuObject.evalMipDimensions(sourceMip);
        for (uint8_t face = 0; face < maxFace; ++face) {
            if (!_gpuObject.isStoredMipFaceAvailable(sourceMip, face)) {
                continue;
            }

            // If the mip is less than the max transfer size, then just do it in one transfer
            if (glm::all(glm::lessThanEqual(mipDimensions, MAX_TRANSFER_DIMENSIONS))) {
                // Can the mip be transferred in one go
                _pendingTransfers.emplace(new TransferJob(*this, sourceMip, targetMip, face));
                continue;
            }

            // break down the transfers into chunks so that no single transfer is 
            // consuming more than X bandwidth
            auto mipSize = _gpuObject.getStoredMipFaceSize(sourceMip, face);
            const auto lines = mipDimensions.y;
            auto bytesPerLine = mipSize / lines;
            Q_ASSERT(0 == (mipSize % lines));
            uint32_t linesPerTransfer = (uint32_t)(MAX_TRANSFER_SIZE / bytesPerLine);
            uint32_t lineOffset = 0;
            while (lineOffset < lines) {
                uint32_t linesToCopy = std::min<uint32_t>(lines - lineOffset, linesPerTransfer);
                _pendingTransfers.emplace(new TransferJob(*this, sourceMip, targetMip, face, linesToCopy, lineOffset));
                lineOffset += linesToCopy;
            }
        }

        // queue up the sampler and populated mip change for after the transfer has completed
        _pendingTransfers.emplace(new TransferJob(*this, [=] {
            _populatedMip = sourceMip;
            syncSampler();
        }));
    } while (sourceMip != _allocatedMip);
}

// resource textures
using GL41ResourceTexture = GL41Backend::GL41ResourceTexture;

GL41ResourceTexture::GL41ResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41VariableAllocationTexture(backend, texture) {
    if (texture.isAutogenerateMips()) {
        generateMips();
    }
}

GL41ResourceTexture::~GL41ResourceTexture() {
}


