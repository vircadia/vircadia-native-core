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
    } else {
        if (texture.getUsageType() == TextureUsageType::RESOURCE) {
            auto varTex = static_cast<GL41VariableAllocationTexture*> (object);

            if (varTex->_minAllocatedMip > 0) {
                auto minAvailableMip = texture.minAvailableMipLevel();
                if (minAvailableMip < varTex->_minAllocatedMip) {
                    varTex->_minAllocatedMip = minAvailableMip;
                    GL41VariableAllocationTexture::_memoryPressureStateStale = true;
                }
            }
        }
    }

    return object;
}

using GL41Texture = GL41Backend::GL41Texture;

GL41Texture::GL41Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture)
    : GLTexture(backend, texture, allocate(texture)) {
}

void GL41Texture::withPreservedTexture(std::function<void()> f) const {
    glActiveTexture(GL_TEXTURE0 + GL41Backend::RESOURCE_TRANSFER_TEX_UNIT);
    glBindTexture(_target, _texture);
    (void)CHECK_GL_ERROR();

    f();
    glBindTexture(_target, 0);
    (void)CHECK_GL_ERROR();
}


GLuint GL41Texture::allocate(const Texture& texture) {
    GLuint result;
    glGenTextures(1, &result);
    return result;
}



void GL41Texture::generateMips() const {
    withPreservedTexture([&] {
        glGenerateMipmap(_target);
    });
    (void)CHECK_GL_ERROR();
}

Size GL41Texture::copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum internalFormat, GLenum format, GLenum type, Size sourceSize, const void* sourcePointer) const {
    Size amountCopied = sourceSize;
    if (GL_TEXTURE_2D == _target) {
        switch (internalFormat) {
            case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_RED_RGTC1:
            case GL_COMPRESSED_RG_RGTC2:
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
                glCompressedTexSubImage2D(_target, mip, 0, yOffset, size.x, size.y, internalFormat,
                                          static_cast<GLsizei>(sourceSize), sourcePointer);
                break;
            default:
                glTexSubImage2D(_target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
                break;
        }
    } else if (GL_TEXTURE_CUBE_MAP == _target) {
        auto target = GLTexture::CUBE_FACE_LAYOUT[face];

        switch (internalFormat) {
            case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_RED_RGTC1:
            case GL_COMPRESSED_RG_RGTC2:
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
                glCompressedTexSubImage2D(target, mip, 0, yOffset, size.x, size.y, internalFormat,
                                          static_cast<GLsizei>(sourceSize), sourcePointer);
                break;
            default:
                glTexSubImage2D(target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
                break;
        }
    } else {
        assert(false);
        amountCopied = 0;
    }
    (void)CHECK_GL_ERROR();
    return amountCopied;
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
    Backend::textureFramebufferCount.increment();
    Backend::textureFramebufferGPUMemSize.update(0, size());
}

GL41AttachmentTexture::~GL41AttachmentTexture() {
    Backend::textureFramebufferCount.decrement();
    Backend::textureFramebufferGPUMemSize.update(size(), 0);
}

// Strict resource textures
using GL41StrictResourceTexture = GL41Backend::GL41StrictResourceTexture;

GL41StrictResourceTexture::GL41StrictResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41FixedAllocationTexture(backend, texture) {
    Backend::textureResidentCount.increment();
    Backend::textureResidentGPUMemSize.update(0, size());

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

GL41StrictResourceTexture::~GL41StrictResourceTexture() {
    Backend::textureResidentCount.decrement();
    Backend::textureResidentGPUMemSize.update(size(), 0);
}


using GL41VariableAllocationTexture = GL41Backend::GL41VariableAllocationTexture;

GL41VariableAllocationTexture::GL41VariableAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) :
     GL41Texture(backend, texture)
{
    Backend::textureResourceCount.increment();

    auto mipLevels = texture.getNumMips();
    _allocatedMip = mipLevels;
    _maxAllocatedMip = _populatedMip = mipLevels;
    _minAllocatedMip = texture.minAvailableMipLevel();

    uvec3 mipDimensions;
    for (uint16_t mip = _minAllocatedMip; mip < mipLevels; ++mip) {
        if (glm::all(glm::lessThanEqual(texture.evalMipDimensions(mip), INITIAL_MIP_TRANSFER_DIMENSIONS))) {
            _maxAllocatedMip = _populatedMip = mip;
            break;
        }
    }

    auto targetMip = _populatedMip - std::min<uint16_t>(_populatedMip, 2);
    uint16_t allocatedMip = std::max<uint16_t>(_minAllocatedMip, targetMip);

    allocateStorage(allocatedMip);
    _memoryPressureStateStale = true;
    copyMipsFromTexture();

    syncSampler();
}

GL41VariableAllocationTexture::~GL41VariableAllocationTexture() {
    Backend::textureResourceCount.decrement();
    Backend::textureResourceGPUMemSize.update(_size, 0);
    Backend::textureResourcePopulatedGPUMemSize.update(_populatedSize, 0);
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
    Backend::textureResourceGPUMemSize.update(0, _size);

}

Size GL41VariableAllocationTexture::copyMipsFromTexture() {
    auto mipLevels = _gpuObject.getNumMips();
    size_t maxFace = GLTexture::getFaceCount(_target);
    Size amount = 0;
    for (uint16_t sourceMip = _populatedMip; sourceMip < mipLevels; ++sourceMip) {
        uint16_t targetMip = sourceMip - _allocatedMip;
        for (uint8_t face = 0; face < maxFace; ++face) {
            amount += copyMipFaceFromTexture(sourceMip, targetMip, face);
        }
    }


    return amount;
}

Size GL41VariableAllocationTexture::copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum internalFormat, GLenum format, GLenum type, Size sourceSize, const void* sourcePointer) const {
    Size amountCopied = 0;
    withPreservedTexture([&] {
        amountCopied = Parent::copyMipFaceLinesFromTexture(mip, face, size, yOffset, internalFormat, format, type, sourceSize, sourcePointer);
    });
    incrementPopulatedSize(amountCopied);
    return amountCopied;
}

void GL41VariableAllocationTexture::syncSampler() const {
    withPreservedTexture([&] {
        Parent::syncSampler();
        glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, _populatedMip - _allocatedMip);
    });
}


void copyUncompressedTexGPUMem(const gpu::Texture& texture, GLenum texTarget, GLuint srcId, GLuint destId, uint16_t numMips, uint16_t srcMipOffset, uint16_t destMipOffset, uint16_t populatedMips) {
    // DestID must be bound to the GL41Backend::RESOURCE_TRANSFER_TEX_UNIT

    GLuint fbo { 0 };
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

    uint16_t mips = numMips;
    // copy pre-existing mips
    for (uint16_t mip = populatedMips; mip < mips; ++mip) {
        auto mipDimensions = texture.evalMipDimensions(mip);
        uint16_t targetMip = mip - destMipOffset;
        uint16_t sourceMip = mip - srcMipOffset;
        for (GLenum target : GLTexture::getFaceTargets(texTarget)) {
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, srcId, sourceMip);
            (void)CHECK_GL_ERROR();
            glCopyTexSubImage2D(target, targetMip, 0, 0, 0, 0, mipDimensions.x, mipDimensions.y);
            (void)CHECK_GL_ERROR();
        }
    }

    // destroy the transfer framebuffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
}

void copyCompressedTexGPUMem(const gpu::Texture& texture, GLenum texTarget, GLuint srcId, GLuint destId, uint16_t numMips, uint16_t srcMipOffset, uint16_t destMipOffset, uint16_t populatedMips) {
    // DestID must be bound to the GL41Backend::RESOURCE_TRANSFER_TEX_UNIT

    struct MipDesc {
        GLint _faceSize;
        GLint _size;
        GLint _offset;
        GLint _width;
        GLint _height;
    };
    std::vector<MipDesc> sourceMips(numMips);

    std::vector<GLubyte> bytes;

    glActiveTexture(GL_TEXTURE0 + GL41Backend::RESOURCE_TRANSFER_EXTRA_TEX_UNIT);
    glBindTexture(texTarget, srcId);
    const auto& faceTargets = GLTexture::getFaceTargets(texTarget);
    GLint internalFormat { 0 };

    // Collect the mip description from the source texture
    GLint bufferOffset { 0 };
    for (uint16_t mip = populatedMips; mip < numMips; ++mip) {
        auto& sourceMip = sourceMips[mip];

        uint16_t sourceLevel = mip - srcMipOffset;

        // Grab internal format once
        if (internalFormat == 0) {
            glGetTexLevelParameteriv(faceTargets[0], sourceLevel, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        }

        // Collect the size of the first face, and then compute the total size offset needed for this mip level
        auto mipDimensions = texture.evalMipDimensions(mip);
        sourceMip._width = mipDimensions.x;
        sourceMip._height = mipDimensions.y;
#ifdef DEBUG_COPY
        glGetTexLevelParameteriv(faceTargets.front(), sourceLevel, GL_TEXTURE_WIDTH, &sourceMip._width);
        glGetTexLevelParameteriv(faceTargets.front(), sourceLevel, GL_TEXTURE_HEIGHT, &sourceMip._height);
#endif
        glGetTexLevelParameteriv(faceTargets.front(), sourceLevel, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &sourceMip._faceSize);
        sourceMip._size = (GLint)faceTargets.size() * sourceMip._faceSize;
        sourceMip._offset = bufferOffset;
        bufferOffset += sourceMip._size;
        gpu::gl::checkGLError();
    }
    (void)CHECK_GL_ERROR();

    // Allocate the PBO to accomodate for all the mips to copy
    GLuint pbo { 0 };
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_PACK_BUFFER, bufferOffset, nullptr, GL_STATIC_COPY);
    (void)CHECK_GL_ERROR();

    // Transfer from source texture to pbo
    for (uint16_t mip = populatedMips; mip < numMips; ++mip) {
        auto& sourceMip = sourceMips[mip];

        uint16_t sourceLevel = mip - srcMipOffset;

        for (GLint f = 0; f < (GLint)faceTargets.size(); f++) {
             glGetCompressedTexImage(faceTargets[f], sourceLevel, BUFFER_OFFSET(sourceMip._offset + f * sourceMip._faceSize));
        }
        (void)CHECK_GL_ERROR();
    }

    // Now populate the new texture from the pbo
    glBindTexture(texTarget, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

    glActiveTexture(GL_TEXTURE0 + GL41Backend::RESOURCE_TRANSFER_TEX_UNIT);

    // Transfer from pbo to new texture
    for (uint16_t mip = populatedMips; mip < numMips; ++mip) {
        auto& sourceMip = sourceMips[mip];

        uint16_t destLevel = mip - destMipOffset;

        for (GLint f = 0; f < (GLint)faceTargets.size(); f++) {
#ifdef DEBUG_COPY
            GLint destWidth, destHeight, destSize;
            glGetTexLevelParameteriv(faceTargets.front(), destLevel, GL_TEXTURE_WIDTH, &destWidth);
            glGetTexLevelParameteriv(faceTargets.front(), destLevel, GL_TEXTURE_HEIGHT, &destHeight);
            glGetTexLevelParameteriv(faceTargets.front(), destLevel, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &destSize);
#endif
            glCompressedTexSubImage2D(faceTargets[f], destLevel, 0, 0, sourceMip._width, sourceMip._height, internalFormat,
                sourceMip._faceSize, BUFFER_OFFSET(sourceMip._offset + f * sourceMip._faceSize));
            gpu::gl::checkGLError();
        }
    }

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glDeleteBuffers(1, &pbo);
}

void GL41VariableAllocationTexture::copyTextureMipsInGPUMem(GLuint srcId, GLuint destId, uint16_t srcMipOffset, uint16_t destMipOffset, uint16_t populatedMips) {
    uint16_t numMips = _gpuObject.getNumMips();
    withPreservedTexture([&] {
        if (_texelFormat.isCompressed()) {
            copyCompressedTexGPUMem(_gpuObject, _target, srcId, destId, numMips, srcMipOffset, destMipOffset, populatedMips);
        } else {
            copyUncompressedTexGPUMem(_gpuObject, _target, srcId, destId, numMips, srcMipOffset, destMipOffset, populatedMips);
        }
    });
}

void GL41VariableAllocationTexture::promote() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    Q_ASSERT(_allocatedMip > 0);

    uint16_t targetAllocatedMip = _allocatedMip - std::min<uint16_t>(_allocatedMip, 2);
    targetAllocatedMip = std::max<uint16_t>(_minAllocatedMip, targetAllocatedMip);

    GLuint oldId = _id;
    auto oldSize = _size;
    uint16_t oldAllocatedMip = _allocatedMip;

    // create new texture
    const_cast<GLuint&>(_id) = allocate(_gpuObject);

    // allocate storage for new level
    allocateStorage(targetAllocatedMip);

    // copy pre-existing mips
    copyTextureMipsInGPUMem(oldId, _id, oldAllocatedMip, _allocatedMip, _populatedMip);

    // destroy the old texture
    glDeleteTextures(1, &oldId);

    // Update sampler
    syncSampler();

    // update the memory usage
    Backend::textureResourceGPUMemSize.update(oldSize, 0);
    // no change to Backend::textureResourcePopulatedGPUMemSize

    populateTransferQueue();
}

void GL41VariableAllocationTexture::demote() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    Q_ASSERT(_allocatedMip < _maxAllocatedMip);
    auto oldId = _id;
    auto oldSize = _size;
    auto oldPopulatedMip = _populatedMip;

    // allocate new texture
    const_cast<GLuint&>(_id) = allocate(_gpuObject);
    uint16_t oldAllocatedMip = _allocatedMip;
    allocateStorage(_allocatedMip + 1);
    _populatedMip = std::max(_populatedMip, _allocatedMip);


    // copy pre-existing mips
    copyTextureMipsInGPUMem(oldId, _id, oldAllocatedMip, _allocatedMip, _populatedMip);

    // destroy the old texture
    glDeleteTextures(1, &oldId);

    // Update sampler
    syncSampler();

    // update the memory usage
    Backend::textureResourceGPUMemSize.update(oldSize, 0);
    // Demoting unpopulate the memory delta
    if (oldPopulatedMip != _populatedMip) {
        auto numPopulatedDemoted = _populatedMip - oldPopulatedMip;
        Size amountUnpopulated = 0;
        for (int i = 0; i < numPopulatedDemoted; i++) {
            amountUnpopulated += _gpuObject.evalMipSize(oldPopulatedMip + i);
        }
        decrementPopulatedSize(amountUnpopulated);
    }
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
            // For compressed format, regions must be a multiple of the 4x4 tiles, so enforce 4 lines as the minimum block
            auto mipSize = _gpuObject.getStoredMipFaceSize(sourceMip, face);
            const auto lines = mipDimensions.y;
            const uint32_t BLOCK_NUM_LINES { 4 };
            const auto numBlocks = (lines + (BLOCK_NUM_LINES - 1)) / BLOCK_NUM_LINES;
            auto bytesPerBlock = mipSize / numBlocks;
            Q_ASSERT(0 == (mipSize % lines));
            uint32_t linesPerTransfer = BLOCK_NUM_LINES * (uint32_t)(MAX_TRANSFER_SIZE / bytesPerBlock);
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

