//
//  GL45BackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GL45Backend.h"
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QDebug>
#include <QtCore/QThread>

#include <NumericalConstants.h>
#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

using GL45VariableAllocationTexture = GL45Backend::GL45VariableAllocationTexture;

GL45VariableAllocationTexture::GL45VariableAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45Texture(backend, texture) {
    ++_frameTexturesCreated;
    Backend::textureResourceCount.increment();
}

GL45VariableAllocationTexture::~GL45VariableAllocationTexture() {
    Backend::textureResourceCount.decrement();
    Backend::textureResourceGPUMemSize.update(_size, 0);
    Backend::textureResourcePopulatedGPUMemSize.update(_populatedSize, 0);
}

Size GL45VariableAllocationTexture::copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum internalFormat, GLenum format, GLenum type, Size sourceSize, const void* sourcePointer) const {
    Size amountCopied = 0;
    amountCopied = Parent::copyMipFaceLinesFromTexture(mip, face, size, yOffset, internalFormat, format, type, sourceSize, sourcePointer);
    incrementPopulatedSize(amountCopied);
    return amountCopied;
}


void copyTexGPUMem(const gpu::Texture& texture, GLenum texTarget, GLuint srcId, GLuint destId, uint16_t numMips, uint16_t srcMipOffset, uint16_t destMipOffset, uint16_t populatedMips) {
    for (uint16_t mip = populatedMips; mip < numMips; ++mip) {
        auto mipDimensions = texture.evalMipDimensions(mip);
        uint16_t targetMip = mip - destMipOffset;
        uint16_t sourceMip = mip - srcMipOffset;
        auto faces = GLTexture::getFaceCount(texTarget);
        for (uint8_t face = 0; face < faces; ++face) {
            glCopyImageSubData(
                srcId, texTarget, sourceMip, 0, 0, face,
                destId, texTarget, targetMip, 0, 0, face,
                mipDimensions.x, mipDimensions.y, 1
                );
            (void)CHECK_GL_ERROR();
        }
    }
}

void GL45VariableAllocationTexture::copyTextureMipsInGPUMem(GLuint srcId, GLuint destId, uint16_t srcMipOffset, uint16_t destMipOffset, uint16_t populatedMips) {
    uint16_t numMips = _gpuObject.getNumMips();
    copyTexGPUMem(_gpuObject, _target, srcId, destId, numMips, srcMipOffset, destMipOffset, populatedMips);
}


// Managed size resource textures
using GL45ResourceTexture = GL45Backend::GL45ResourceTexture;

GL45ResourceTexture::GL45ResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45VariableAllocationTexture(backend, texture) {
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

void GL45ResourceTexture::allocateStorage(uint16 allocatedMip) {
    _allocatedMip = allocatedMip;
    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const auto dimensions = _gpuObject.evalMipDimensions(_allocatedMip);
    const auto totalMips = _gpuObject.getNumMips();
    const auto mips = totalMips - _allocatedMip;
    glTextureStorage2D(_id, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);
    auto mipLevels = _gpuObject.getNumMips();
    _size = 0;
    for (uint16_t mip = _allocatedMip; mip < mipLevels; ++mip) {
        _size += _gpuObject.evalMipSize(mip);
    }
    Backend::textureResourceGPUMemSize.update(0, _size);
}

Size GL45ResourceTexture::copyMipsFromTexture() {
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

void GL45ResourceTexture::syncSampler() const {
    Parent::syncSampler();
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, _populatedMip - _allocatedMip);
}

void GL45ResourceTexture::promote() {
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

void GL45ResourceTexture::demote() {
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

void GL45ResourceTexture::populateTransferQueue() {
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

// Sparsely allocated, managed size resource textures
#if 0
#define SPARSE_PAGE_SIZE_OVERHEAD_ESTIMATE 1.3f

using GL45SparseResourceTexture = GL45Backend::GL45SparseResourceTexture;

GL45Texture::PageDimensionsMap GL45Texture::pageDimensionsByFormat;
Mutex GL45Texture::pageDimensionsMutex;

GL45Texture::PageDimensions GL45Texture::getPageDimensionsForFormat(const TextureTypeFormat& typeFormat) {
    {
        Lock lock(pageDimensionsMutex);
        if (pageDimensionsByFormat.count(typeFormat)) {
            return pageDimensionsByFormat[typeFormat];
        }
    }

    GLint count = 0;
    glGetInternalformativ(typeFormat.first, typeFormat.second, GL_NUM_VIRTUAL_PAGE_SIZES_ARB, 1, &count);

    std::vector<uvec3> result;
    if (count > 0) {
        std::vector<GLint> x, y, z;
        x.resize(count);
        glGetInternalformativ(typeFormat.first, typeFormat.second, GL_VIRTUAL_PAGE_SIZE_X_ARB, 1, &x[0]);
        y.resize(count);
        glGetInternalformativ(typeFormat.first, typeFormat.second, GL_VIRTUAL_PAGE_SIZE_Y_ARB, 1, &y[0]);
        z.resize(count);
        glGetInternalformativ(typeFormat.first, typeFormat.second, GL_VIRTUAL_PAGE_SIZE_Z_ARB, 1, &z[0]);

        result.resize(count);
        for (GLint i = 0; i < count; ++i) {
            result[i] = uvec3(x[i], y[i], z[i]);
        }
    }

    {
        Lock lock(pageDimensionsMutex);
        if (0 == pageDimensionsByFormat.count(typeFormat)) {
            pageDimensionsByFormat[typeFormat] = result;
        }
    }

    return result;
}

GL45Texture::PageDimensions GL45Texture::getPageDimensionsForFormat(GLenum target, GLenum format) {
    return getPageDimensionsForFormat({ target, format });
}
bool GL45Texture::isSparseEligible(const Texture& texture) {
    Q_ASSERT(TextureUsageType::RESOURCE == texture.getUsageType());

    // Disabling sparse for the momemnt
    return false;

    const auto allowedPageDimensions = getPageDimensionsForFormat(getGLTextureType(texture), 
        gl::GLTexelFormat::evalGLTexelFormatInternal(texture.getTexelFormat()));
    const auto textureDimensions = texture.getDimensions();
    for (const auto& pageDimensions : allowedPageDimensions) {
        if (uvec3(0) == (textureDimensions % pageDimensions)) {
            return true;
        }
    }

    return false;
}


GL45SparseResourceTexture::GL45SparseResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45VariableAllocationTexture(backend, texture) {
    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const uvec3 dimensions = _gpuObject.getDimensions();
    auto allowedPageDimensions = getPageDimensionsForFormat(_target, texelFormat.internalFormat);
    uint32_t pageDimensionsIndex = 0;
    // In order to enable sparse the texture size must be an integer multiple of the page size
    for (size_t i = 0; i < allowedPageDimensions.size(); ++i) {
        pageDimensionsIndex = (uint32_t)i;
        _pageDimensions = allowedPageDimensions[i];
        // Is this texture an integer multiple of page dimensions?
        if (uvec3(0) == (dimensions % _pageDimensions)) {
            qCDebug(gpugl45logging) << "Enabling sparse for texture " << _gpuObject.source().c_str();
            break;
        }
    }
    glTextureParameteri(_id, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
    glTextureParameteri(_id, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, pageDimensionsIndex);
    glGetTextureParameterIuiv(_id, GL_NUM_SPARSE_LEVELS_ARB, &_maxSparseLevel);

    _pageBytes = _gpuObject.getTexelFormat().getSize();
    _pageBytes *= _pageDimensions.x * _pageDimensions.y * _pageDimensions.z;
    // Testing with a simple texture allocating app shows an estimated 20% GPU memory overhead for 
    // sparse textures as compared to non-sparse, so we acount for that here.
    _pageBytes = (uint32_t)(_pageBytes * SPARSE_PAGE_SIZE_OVERHEAD_ESTIMATE);

    //allocateStorage();
    syncSampler();
}

GL45SparseResourceTexture::~GL45SparseResourceTexture() {
    Backend::updateTextureGPUVirtualMemoryUsage(size(), 0);
}

uvec3 GL45SparseResourceTexture::getPageCounts(const uvec3& dimensions) const {
    auto result = (dimensions / _pageDimensions) +
        glm::clamp(dimensions % _pageDimensions, glm::uvec3(0), glm::uvec3(1));
    return result;
}

uint32_t GL45SparseResourceTexture::getPageCount(const uvec3& dimensions) const {
    auto pageCounts = getPageCounts(dimensions);
    return pageCounts.x * pageCounts.y * pageCounts.z;
}

void GL45SparseResourceTexture::promote() {
}

void GL45SparseResourceTexture::demote() {
}

SparseInfo::SparseInfo(GL45Texture& texture)
    : texture(texture) {
}

void SparseInfo::maybeMakeSparse() {
    // Don't enable sparse for objects with explicitly managed mip levels
    if (!texture._gpuObject.isAutogenerateMips()) {
        return;
    }

    const uvec3 dimensions = texture._gpuObject.getDimensions();
    auto allowedPageDimensions = getPageDimensionsForFormat(texture._target, texture._internalFormat);
    // In order to enable sparse the texture size must be an integer multiple of the page size
    for (size_t i = 0; i < allowedPageDimensions.size(); ++i) {
        pageDimensionsIndex = (uint32_t)i;
        pageDimensions = allowedPageDimensions[i];
        // Is this texture an integer multiple of page dimensions?
        if (uvec3(0) == (dimensions % pageDimensions)) {
            qCDebug(gpugl45logging) << "Enabling sparse for texture " << texture._source.c_str();
            sparse = true;
            break;
        }
    }

    if (sparse) {
        glTextureParameteri(texture._id, GL_TEXTURE_SPARSE_ARB, GL_TRUE);
        glTextureParameteri(texture._id, GL_VIRTUAL_PAGE_SIZE_INDEX_ARB, pageDimensionsIndex);
    } else {
        qCDebug(gpugl45logging) << "Size " << dimensions.x << " x " << dimensions.y <<
            " is not supported by any sparse page size for texture" << texture._source.c_str();
    }
}


// This can only be called after we've established our storage size
void SparseInfo::update() {
    if (!sparse) {
        return;
    }
    glGetTextureParameterIuiv(texture._id, GL_NUM_SPARSE_LEVELS_ARB, &maxSparseLevel);

    for (uint16_t mipLevel = 0; mipLevel <= maxSparseLevel; ++mipLevel) {
        auto mipDimensions = texture._gpuObject.evalMipDimensions(mipLevel);
        auto mipPageCount = getPageCount(mipDimensions);
        maxPages += mipPageCount;
    }
    if (texture._target == GL_TEXTURE_CUBE_MAP) {
        maxPages *= GLTexture::CUBE_NUM_FACES;
    }
}


void SparseInfo::allocateToMip(uint16_t targetMip) {
    // Not sparse, do nothing
    if (!sparse) {
        return;
    }

    if (allocatedMip == INVALID_MIP) {
        allocatedMip = maxSparseLevel + 1;
    }

    // Don't try to allocate below the maximum sparse level
    if (targetMip > maxSparseLevel) {
        targetMip = maxSparseLevel;
    }

    // Already allocated this level
    if (allocatedMip <= targetMip) {
        return;
    }

    uint32_t maxFace = (uint32_t)(GL_TEXTURE_CUBE_MAP == texture._target ? CUBE_NUM_FACES : 1);
    for (uint16_t mip = targetMip; mip < allocatedMip; ++mip) {
        auto size = texture._gpuObject.evalMipDimensions(mip);
        glTexturePageCommitmentEXT(texture._id, mip, 0, 0, 0, size.x, size.y, maxFace, GL_TRUE);
        allocatedPages += getPageCount(size);
    }
    allocatedMip = targetMip;
}

uint32_t SparseInfo::getSize() const {
    return allocatedPages * pageBytes;
}
using SparseInfo = GL45Backend::GL45Texture::SparseInfo;

void GL45Texture::updateSize() const {
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }

    if (_transferrable && _sparseInfo.sparse) {
        auto size = _sparseInfo.getSize();
        Backend::updateTextureGPUSparseMemoryUsage(_size, size);
        setSize(size);
    } else {
        setSize(_gpuObject.evalTotalSize(_mipOffset));
    }
}

void GL45Texture::startTransfer() {
    Parent::startTransfer();
    _sparseInfo.update();
    _populatedMip = _maxMip + 1;
}

bool GL45Texture::continueTransfer() {
    size_t maxFace = GL_TEXTURE_CUBE_MAP == _target ? CUBE_NUM_FACES : 1;
    if (_populatedMip == _minMip) {
        return false;
    }

    uint16_t targetMip = _populatedMip - 1;
    while (targetMip > 0 && !_gpuObject.isStoredMipFaceAvailable(targetMip)) {
        --targetMip;
    }

    _sparseInfo.allocateToMip(targetMip);
    for (uint8_t face = 0; face < maxFace; ++face) {
        auto size = _gpuObject.evalMipDimensions(targetMip);
        if (_gpuObject.isStoredMipFaceAvailable(targetMip, face)) {
            auto mip = _gpuObject.accessStoredMipFace(targetMip, face);
            GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), mip->getFormat());
            if (GL_TEXTURE_2D == _target) {
                glTextureSubImage2D(_id, targetMip, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
            } else if (GL_TEXTURE_CUBE_MAP == _target) {
                // DSA ARB does not work on AMD, so use EXT
                // unless EXT is not available on the driver
                if (glTextureSubImage2DEXT) {
                    auto target = CUBE_FACE_LAYOUT[face];
                    glTextureSubImage2DEXT(_id, target, targetMip, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
                } else {
                    glTextureSubImage3D(_id, targetMip, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mip->readData());
                }
            } else {
                Q_ASSERT(false);
            }
            (void)CHECK_GL_ERROR();
            break;
        }
    }
    _populatedMip = targetMip;
    return _populatedMip != _minMip;
}

void GL45Texture::finishTransfer() {
    Parent::finishTransfer();
}

void GL45Texture::postTransfer() {
    Parent::postTransfer();
}

void GL45Texture::stripToMip(uint16_t newMinMip) {
    if (newMinMip < _minMip) {
        qCWarning(gpugl45logging) << "Cannot decrease the min mip";
        return;
    }

    if (_sparseInfo.sparse && newMinMip > _sparseInfo.maxSparseLevel) {
        qCWarning(gpugl45logging) << "Cannot increase the min mip into the mip tail";
        return;
    }

    // If we weren't generating mips before, we need to now that we're stripping down mip levels.
    if (!_gpuObject.isAutogenerateMips()) {
        qCDebug(gpugl45logging) << "Force mip generation for texture";
        glGenerateTextureMipmap(_id);
    }


    uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
    if (_sparseInfo.sparse) {
        for (uint16_t mip = _minMip; mip < newMinMip; ++mip) {
            auto id = _id;
            auto mipDimensions = _gpuObject.evalMipDimensions(mip);
            glTexturePageCommitmentEXT(id, mip, 0, 0, 0, mipDimensions.x, mipDimensions.y, maxFace, GL_FALSE);
            auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions) * maxFace;
            assert(deallocatedPages < _sparseInfo.allocatedPages);
            _sparseInfo.allocatedPages -= deallocatedPages;
        }
        _minMip = newMinMip;
    } else {
        GLuint oldId = _id;
        // Find the distance between the old min mip and the new one
        uint16 mipDelta = newMinMip - _minMip;
        _mipOffset += mipDelta;
        const_cast<uint16&>(_maxMip) -= mipDelta;
        auto newLevels = usedMipLevels();

        // Create and setup the new texture (allocate)
        glCreateTextures(_target, 1, &const_cast<GLuint&>(_id));
        glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
        glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
        Vec3u newDimensions = _gpuObject.evalMipDimensions(_mipOffset);
        glTextureStorage2D(_id, newLevels, _internalFormat, newDimensions.x, newDimensions.y);

        // Copy the contents of the old texture to the new
        GLuint fbo { 0 };
        glCreateFramebuffers(1, &fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        for (uint16 targetMip = _minMip; targetMip <= _maxMip; ++targetMip) {
            uint16 sourceMip = targetMip + mipDelta;
            Vec3u mipDimensions = _gpuObject.evalMipDimensions(targetMip + _mipOffset);
            for (GLenum target : getFaceTargets(_target)) {
                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, oldId, sourceMip);
                (void)CHECK_GL_ERROR();
                glCopyTextureSubImage2D(_id, targetMip, 0, 0, 0, 0, mipDimensions.x, mipDimensions.y);
                (void)CHECK_GL_ERROR();
            }
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(1, &oldId);
    }

    // Re-sync the sampler to force access to the new mip level
    syncSampler();
    updateSize();
}

bool GL45Texture::derezable() const {
    if (_external) {
        return false;
    }
    auto maxMinMip = _sparseInfo.sparse ? _sparseInfo.maxSparseLevel : _maxMip;
    return _transferrable && (_targetMinMip < maxMinMip);
}

size_t GL45Texture::getMipByteCount(uint16_t mip) const {
    if (!_sparseInfo.sparse) {
        return Parent::getMipByteCount(mip);
    }

    auto dimensions = _gpuObject.evalMipDimensions(_targetMinMip);
    return _sparseInfo.getPageCount(dimensions) * _sparseInfo.pageBytes;
}

std::pair<size_t, bool> GL45Texture::preDerez() {
    assert(!_sparseInfo.sparse || _targetMinMip < _sparseInfo.maxSparseLevel);
    size_t freedMemory = getMipByteCount(_targetMinMip);
    bool liveMip = _populatedMip != INVALID_MIP && _populatedMip <= _targetMinMip;
    ++_targetMinMip;
    return { freedMemory, liveMip };
}

void GL45Texture::derez() {
    if (_sparseInfo.sparse) {
        assert(_minMip < _sparseInfo.maxSparseLevel);
    }
    assert(_minMip < _maxMip);
    assert(_transferrable);
    stripToMip(_minMip + 1);
}

size_t GL45Texture::getCurrentGpuSize() const {
    if (!_sparseInfo.sparse) {
        return Parent::getCurrentGpuSize();
    }

    return _sparseInfo.getSize();
}

size_t GL45Texture::getTargetGpuSize() const {
    if (!_sparseInfo.sparse) {
        return Parent::getTargetGpuSize();
    }

    size_t result = 0;
    for (auto mip = _targetMinMip; mip <= _sparseInfo.maxSparseLevel; ++mip) {
        result += (_sparseInfo.pageBytes * _sparseInfo.getPageCount(_gpuObject.evalMipDimensions(mip)));
    }

    return result;
}

GL45Texture::~GL45Texture() {
    if (_sparseInfo.sparse) {
        uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
        auto maxSparseMip = std::min<uint16_t>(_maxMip, _sparseInfo.maxSparseLevel);
        for (uint16_t mipLevel = _minMip; mipLevel <= maxSparseMip; ++mipLevel) {
            auto mipDimensions = _gpuObject.evalMipDimensions(mipLevel);
            glTexturePageCommitmentEXT(_texture, mipLevel, 0, 0, 0, mipDimensions.x, mipDimensions.y, maxFace, GL_FALSE);
            auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions) * maxFace;
            assert(deallocatedPages <= _sparseInfo.allocatedPages);
            _sparseInfo.allocatedPages -= deallocatedPages;
        }

        if (0 != _sparseInfo.allocatedPages) {
            qCWarning(gpugl45logging) << "Allocated pages remaining " << _id << " " << _sparseInfo.allocatedPages;
        }
        Backend::decrementTextureGPUSparseCount();
    }
}
GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture)
    : GLTexture(backend, texture, allocate(texture)), _sparseInfo(*this), _targetMinMip(_minMip)
{

    auto theBackend = _backend.lock();
    if (_transferrable && theBackend && theBackend->isTextureManagementSparseEnabled()) {
        _sparseInfo.maybeMakeSparse();
        if (_sparseInfo.sparse) {
            Backend::incrementTextureGPUSparseCount();
        }
    }
}
#endif
