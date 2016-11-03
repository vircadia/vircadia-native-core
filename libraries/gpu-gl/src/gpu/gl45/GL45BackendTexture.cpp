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
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QDebug>
#include <QtCore/QThread>

#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

// Allocate 1 MB of buffer space for paged transfers
#define DEFAULT_PAGE_BUFFER_SIZE (1024*1024)
#define DEFAULT_GL_PIXEL_ALIGNMENT 4

using GL45Texture = GL45Backend::GL45Texture;

static std::map<uint16_t, std::unordered_set<GL45Texture*>> texturesByMipCounts;
static Mutex texturesByMipCountsMutex;
using TextureTypeFormat = std::pair<GLenum, GLenum>;
std::map<TextureTypeFormat, std::vector<uvec3>> sparsePageDimensionsByFormat;
Mutex sparsePageDimensionsByFormatMutex;

static std::vector<uvec3> getPageDimensionsForFormat(const TextureTypeFormat& typeFormat) {
    {
        Lock lock(sparsePageDimensionsByFormatMutex);
        if (sparsePageDimensionsByFormat.count(typeFormat)) {
            return sparsePageDimensionsByFormat[typeFormat];
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
        qCDebug(gpugl45logging) << "Got " << count << " page sizes";
    }

    {
        Lock lock(sparsePageDimensionsByFormatMutex);
        if (0 == sparsePageDimensionsByFormat.count(typeFormat)) {
            sparsePageDimensionsByFormat[typeFormat] = result;
        }
    }

    return result;
}

static std::vector<uvec3> getPageDimensionsForFormat(GLenum target, GLenum format) {
    return getPageDimensionsForFormat({ target, format });
}

GLTexture* GL45Backend::syncGPUObject(const TexturePointer& texture, bool transfer) {
    return GL45Texture::sync<GL45Texture>(*this, texture, transfer);
}

using SparseInfo = GL45Backend::GL45Texture::SparseInfo;

SparseInfo::SparseInfo(GL45Texture& texture)
    : texture(texture) {
}

void SparseInfo::maybeMakeSparse() {
    // Don't enable sparse for objects with explicitly managed mip levels
    if (!texture._gpuObject.isAutogenerateMips()) {
        qCDebug(gpugl45logging) << "Don't enable sparse texture for explicitly generated mipmaps on texture " << texture._source.c_str();
        return;
    }

    const uvec3 dimensions = texture._gpuObject.getDimensions();
    auto allowedPageDimensions = getPageDimensionsForFormat(texture._target, texture._internalFormat);
    // In order to enable sparse the texture size must be an integer multiple of the page size
    for (size_t i = 0; i < allowedPageDimensions.size(); ++i) {
        pageDimensionsIndex = (uint32_t) i;
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
    pageBytes = texture._gpuObject.getTexelFormat().getSize();
    pageBytes *= pageDimensions.x * pageDimensions.y * pageDimensions.z;

    for (uint16_t mipLevel = 0; mipLevel <= maxSparseLevel; ++mipLevel) {
        auto mipDimensions = texture._gpuObject.evalMipDimensions(mipLevel);
        auto mipPageCount = getPageCount(mipDimensions);
        maxPages += mipPageCount;
    }
    if (texture._target == GL_TEXTURE_CUBE_MAP) {
        maxPages *= GLTexture::CUBE_NUM_FACES;
    }
}

uvec3 SparseInfo::getPageCounts(const uvec3& dimensions) const {
    auto result = (dimensions / pageDimensions) +
        glm::clamp(dimensions % pageDimensions, glm::uvec3(0), glm::uvec3(1));
    return result;
}

uint32_t SparseInfo::getPageCount(const uvec3& dimensions) const {
    auto pageCounts = getPageCounts(dimensions);
    return pageCounts.x * pageCounts.y * pageCounts.z;
}



void GL45Backend::initTextureManagementStage() {

    // enable the Sparse Texture on gl45
    _textureManagement._sparseCapable = true;
    _textureManagement._incrementalTransferCapable = true;

    // But now let s refine the behavior based on vendor
    std::string vendor { (const char*)glGetString(GL_VENDOR) };
    if ((vendor.compare("AMD") <= 0) || (vendor.compare("INTEL") <= 0)) {
        qCDebug(gpugllogging) << "GPU is sparse capable but force it off, vendor = " << vendor.c_str();
        _textureManagement._sparseCapable = false;
    } else {
        qCDebug(gpugllogging) << "GPU is sparse capable, vendor = " << vendor.c_str();
    }
}

using TransferState = GL45Backend::GL45Texture::TransferState;

TransferState::TransferState(GL45Texture& texture) : texture(texture) {
}

void TransferState::updateMip() {
    mipDimensions = texture._gpuObject.evalMipDimensions(mipLevel);
    mipOffset = uvec3();
    if (!texture._gpuObject.isStoredMipFaceAvailable(mipLevel, face)) {
        srcPointer = nullptr;
        return;
    }

    auto mip = texture._gpuObject.accessStoredMipFace(mipLevel, face);
    texelFormat = gl::GLTexelFormat::evalGLTexelFormat(texture._gpuObject.getTexelFormat(), mip->getFormat());
    srcPointer = mip->readData();
    bytesPerLine = (uint32_t)mip->getSize() / mipDimensions.y;
    bytesPerPixel = bytesPerLine / mipDimensions.x;
}

bool TransferState::increment() {
    const SparseInfo& sparse = texture._sparseInfo;
    if ((mipOffset.x + sparse.pageDimensions.x) < mipDimensions.x) {
        mipOffset.x += sparse.pageDimensions.x;
        return true;
    }

    if ((mipOffset.y + sparse.pageDimensions.y) < mipDimensions.y) {
        mipOffset.x = 0;
        mipOffset.y += sparse.pageDimensions.y;
        return true;
    }

    if (mipOffset.z + sparse.pageDimensions.z < mipDimensions.z) {
        mipOffset.x = 0;
        mipOffset.y = 0;
        ++mipOffset.z;
        return true;
    }

    // Done with this mip?, move on to the next mip 
    if (mipLevel + 1 < texture.usedMipLevels()) {
        mipOffset = uvec3(0);
        ++mipLevel;
        updateMip();
        return true;
    }

    uint8_t maxFace = (uint8_t)((texture._target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
    uint8_t nextFace = face + 1;
    // Done with this face?  Move on to the next
    if (nextFace < maxFace) {
        ++face;
        mipOffset = uvec3(0);
        mipLevel = 0;
        updateMip();
        return true;
    }

    return false;
}

void TransferState::populatePage(std::vector<uint8_t>& buffer) {
    uvec3 pageSize = currentPageSize();
    auto bytesPerPageLine = bytesPerPixel * pageSize.x;
    if (0 != (bytesPerPageLine % DEFAULT_GL_PIXEL_ALIGNMENT)) {
        bytesPerPageLine += DEFAULT_GL_PIXEL_ALIGNMENT - (bytesPerPageLine % DEFAULT_GL_PIXEL_ALIGNMENT);
        assert(0 == (bytesPerPageLine % DEFAULT_GL_PIXEL_ALIGNMENT));
    }
    auto totalPageSize = bytesPerPageLine * pageSize.y;
    if (totalPageSize > buffer.size()) {
        buffer.resize(totalPageSize);
    }
    uint8_t* dst = &buffer[0];
    for (uint32_t y = 0; y < pageSize.y; ++y) {
        uint32_t srcOffset = (bytesPerLine * (mipOffset.y + y)) + (bytesPerPixel * mipOffset.x);
        uint32_t dstOffset = bytesPerPageLine * y;
        memcpy(dst + dstOffset, srcPointer + srcOffset, pageSize.x * bytesPerPixel);
    }
}

uvec3 TransferState::currentPageSize() const {
    return glm::clamp(mipDimensions - mipOffset, uvec3(1), texture._sparseInfo.pageDimensions);
}

GLuint GL45Texture::allocate(const Texture& texture) {
    GLuint result;
    glCreateTextures(getGLTextureType(texture), 1, &result);
    return result;
}

GLuint GL45Backend::getTextureID(const TexturePointer& texture, bool transfer) {
    return GL45Texture::getId<GL45Texture>(*this, texture, transfer);
}

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint externalId)
    : GLTexture(backend, texture, externalId), _sparseInfo(*this), _transferState(*this) {
}

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, bool transferrable)
    : GLTexture(backend, texture, allocate(texture), transferrable), _sparseInfo(*this), _transferState(*this) {

    auto theBackend = _backend.lock();
    if (_transferrable && theBackend && theBackend->isTextureManagementSparseEnabled()) {
        _sparseInfo.maybeMakeSparse();
        if (_sparseInfo.sparse) {
            Backend::incrementTextureGPUSparseCount();
        }
    }
}

GL45Texture::~GL45Texture() {
    // External textures cycle very quickly, so don't spam the log with messages about them.
    if (!_gpuObject.getUsage().isExternal()) {
        qCDebug(gpugl45logging) << "Destroying texture " << _id << " from source " << _source.c_str();
    }
    if (_sparseInfo.sparse) {
        Backend::decrementTextureGPUSparseCount();
        // Remove this texture from the candidate list of derezzable textures
        {
            auto mipLevels = usedMipLevels();
            Lock lock(texturesByMipCountsMutex);
            if (texturesByMipCounts.count(mipLevels)) {
                auto& textures = texturesByMipCounts[mipLevels];
                textures.erase(this);
                if (textures.empty()) {
                    texturesByMipCounts.erase(mipLevels);
                }
            }
        }

        // Experimenation suggests that allocating sparse textures on one context/thread and deallocating 
        // them on another is buggy.  So for sparse textures we need to queue a lambda with the deallocation 
        // callls to the transfer thread
        auto id = _id;
        // Set the class _id to 0 so we don't try to double delete
        const_cast<GLuint&>(_id) = 0;
        std::list<std::function<void()>> destructionFunctions;
        
        uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
        auto maxSparseMip = std::min<uint16_t>(_maxMip, _sparseInfo.maxSparseLevel);
        for (uint16_t mipLevel = _minMip; mipLevel <= maxSparseMip; ++mipLevel) {
            auto mipDimensions = _gpuObject.evalMipDimensions(mipLevel);
            destructionFunctions.push_back([id, maxFace, mipLevel, mipDimensions] {
                glTexturePageCommitmentEXT(id, mipLevel, 0, 0, 0, mipDimensions.x, mipDimensions.y, maxFace, GL_FALSE);
            });

            auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions) * maxFace;
            assert(deallocatedPages <= _allocatedPages);
            _allocatedPages -= deallocatedPages;
        }

        if (0 != _allocatedPages) {
            qCWarning(gpugl45logging) << "Allocated pages remaining " << _id << " " << _allocatedPages;
        }

        auto size = _size;
        const_cast<GLuint&>(_size) = 0;
        _textureTransferHelper->queueExecution([id, size, destructionFunctions] {
            for (auto function : destructionFunctions) {
                function();
            }
            glDeleteTextures(1, &id);
            Backend::decrementTextureGPUCount();
            Backend::updateTextureGPUMemoryUsage(size, 0);
            Backend::updateTextureGPUSparseMemoryUsage(size, 0);
        });
    }
}

void GL45Texture::withPreservedTexture(std::function<void()> f) const {
    f();
}

void GL45Texture::generateMips() const {
    if (_transferrable) {
        qCDebug(gpugl45logging) << "Generating mipmaps for " << _source.c_str();
    }
    glGenerateTextureMipmap(_id);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::allocateStorage() const {
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    // Get the dimensions, accounting for the downgrade level
    Vec3u dimensions = _gpuObject.evalMipDimensions(_minMip);
    glTextureStorage2D(_id, usedMipLevels(), _internalFormat, dimensions.x, dimensions.y);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::updateSize() const {
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }

    if (_transferrable && _sparseInfo.sparse) {
        auto size = _allocatedPages * _sparseInfo.pageBytes;
        Backend::updateTextureGPUSparseMemoryUsage(_size, size);
        setSize(_allocatedPages * _sparseInfo.pageBytes);
    } else {
        setSize(_virtualSize);
    }
}

void GL45Texture::startTransfer() {
    Parent::startTransfer();
    _sparseInfo.update();
    _transferState.updateMip();
}

bool GL45Texture::continueTransfer() {
    auto backend = _backend.lock();
    if (!backend || !backend->isTextureManagementIncrementalTransferEnabled()) {
        size_t maxFace = GL_TEXTURE_CUBE_MAP == _target ? CUBE_NUM_FACES : 1;
        for (uint8_t face = 0; face < maxFace; ++face) {
            for (uint16_t mipLevel = _minMip; mipLevel <= _maxMip; ++mipLevel) {
                if (_gpuObject.isStoredMipFaceAvailable(mipLevel, face)) {
                    auto mip = _gpuObject.accessStoredMipFace(mipLevel, face);
                    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), mip->getFormat());
                    auto size = _gpuObject.evalMipDimensions(mipLevel);
                    if (GL_TEXTURE_2D == _target) {
                        glTextureSubImage2D(_id, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
                    } else if (GL_TEXTURE_CUBE_MAP == _target) {
                        // DSA ARB does not work on AMD, so use EXT
                        // glTextureSubImage3D(_id, mipLevel, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mip->readData());
                        auto target = CUBE_FACE_LAYOUT[face];
                        glTextureSubImage2DEXT(_id, target, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
                    } else {
                        Q_ASSERT(false);
                    }
                    (void)CHECK_GL_ERROR();
                }
            }
        }
        return false;
    }

    static std::vector<uint8_t> buffer;
    if (buffer.empty()) {
        buffer.resize(DEFAULT_PAGE_BUFFER_SIZE);
    }
    const uvec3 pageSize = _transferState.currentPageSize();
    const uvec3& offset = _transferState.mipOffset;

    if (_sparseInfo.sparse && _transferState.mipLevel <= _sparseInfo.maxSparseLevel) {
        if (_allocatedPages > _sparseInfo.maxPages) {
            qCWarning(gpugl45logging) << "Exceeded max page allocation!";
        }
        glTexturePageCommitmentEXT(_id, _transferState.mipLevel,
            offset.x, offset.y, _transferState.face,
            pageSize.x, pageSize.y, pageSize.z,
            GL_TRUE);
        ++_allocatedPages;
    }

    if (_transferState.srcPointer) {
        // Transfer the mip data
        _transferState.populatePage(buffer);
        if (GL_TEXTURE_2D == _target) {
            glTextureSubImage2D(_id, _transferState.mipLevel,
                offset.x, offset.y,
                pageSize.x, pageSize.y,
                _transferState.texelFormat.format, _transferState.texelFormat.type, &buffer[0]);
        } else if (GL_TEXTURE_CUBE_MAP == _target) {
            auto target = CUBE_FACE_LAYOUT[_transferState.face];
            // DSA ARB does not work on AMD, so use EXT
            // glTextureSubImage3D(_id, mipLevel, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mip->readData());
            glTextureSubImage2DEXT(_id, target, _transferState.mipLevel,
                offset.x, offset.y,
                pageSize.x, pageSize.y,
                _transferState.texelFormat.format, _transferState.texelFormat.type, &buffer[0]);
        }
    }

    serverWait();
    auto currentMip = _transferState.mipLevel;
    auto result = _transferState.increment();
    if (_sparseInfo.sparse && _transferState.mipLevel != currentMip && currentMip <= _sparseInfo.maxSparseLevel) {
        auto mipDimensions = _gpuObject.evalMipDimensions(currentMip);
        auto mipExpectedPages = _sparseInfo.getPageCount(mipDimensions);
        auto newPages = _allocatedPages - _lastMipAllocatedPages;
        if (newPages != mipExpectedPages) {
            qCWarning(gpugl45logging) << "Unexpected page allocation size... " << newPages << " " << mipExpectedPages;
        }
        _lastMipAllocatedPages = _allocatedPages;
    }
    return result;
}

void GL45Texture::finishTransfer() {
    Parent::finishTransfer();
}

void GL45Texture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();

    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    if (sampler.doComparison()) {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_FUNC, COMPARISON_TO_GL[sampler.getComparisonFunction()]);
    } else {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, WRAP_MODES[sampler.getWrapModeU()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, WRAP_MODES[sampler.getWrapModeV()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_R, WRAP_MODES[sampler.getWrapModeW()]);
    glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), _minMip);
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
    glTextureParameterf(_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
}

void GL45Texture::postTransfer() {
    Parent::postTransfer();
    if (_sparseInfo.sparse) {
        auto mipLevels = usedMipLevels();
        if (mipLevels > 1 && _minMip < _sparseInfo.maxSparseLevel) {
            Lock lock(texturesByMipCountsMutex);
            texturesByMipCounts[mipLevels].insert(this);
        }
    }
}

void GL45Texture::stripToMip(uint16_t newMinMip) {
    if (!_sparseInfo.sparse) {
        return;
    }

    if (newMinMip < _minMip) {
        qCWarning(gpugl45logging) << "Cannot decrease the min mip";
        return;
    }

    if (newMinMip > _sparseInfo.maxSparseLevel) {
        qCWarning(gpugl45logging) << "Cannot increase the min mip into the mip tail";
        return;
    }

    auto mipLevels = usedMipLevels();
    {
        Lock lock(texturesByMipCountsMutex);
        assert(0 != texturesByMipCounts.count(mipLevels));
        assert(0 != texturesByMipCounts[mipLevels].count(this));
        texturesByMipCounts[mipLevels].erase(this);
        if (texturesByMipCounts[mipLevels].empty()) {
            texturesByMipCounts.erase(mipLevels);
        }
    }

    // If we weren't generating mips before, we need to now that we're stripping down mip levels.
    if (!_gpuObject.isAutogenerateMips()) {
        qCDebug(gpugl45logging) << "Force mip generation for texture";
        glGenerateTextureMipmap(_id);
    }


    uint8_t maxFace = (uint8_t)((_target == GL_TEXTURE_CUBE_MAP) ? GLTexture::CUBE_NUM_FACES : 1);
    for (uint16_t mip = _minMip; mip < newMinMip; ++mip) {
        auto id = _id;
        auto mipDimensions = _gpuObject.evalMipDimensions(mip);
        _textureTransferHelper->queueExecution([id, mip, mipDimensions, maxFace] {
            glTexturePageCommitmentEXT(id, mip, 0, 0, 0, mipDimensions.x, mipDimensions.y, maxFace, GL_FALSE);
        });

        auto deallocatedPages = _sparseInfo.getPageCount(mipDimensions) * maxFace;
        assert(deallocatedPages < _allocatedPages);
        _allocatedPages -= deallocatedPages;
    }

    _minMip = newMinMip;
    // Re-sync the sampler to force access to the new mip level
    syncSampler();
    updateSize();


    // Re-insert into the texture-by-mips map if appropriate
    mipLevels = usedMipLevels();
    if (_sparseInfo.sparse && mipLevels > 1 && _minMip < _sparseInfo.maxSparseLevel) {
        Lock lock(texturesByMipCountsMutex);
        texturesByMipCounts[mipLevels].insert(this);
    }
}

void GL45Texture::updateMips() {
    if (!_sparseInfo.sparse) {
        return;
    }
    auto newMinMip = std::min<uint16_t>(_gpuObject.minMip(), _sparseInfo.maxSparseLevel);
    if (_minMip < newMinMip) {
        stripToMip(newMinMip);
    }
}

void GL45Texture::derez() {
    assert(_sparseInfo.sparse);
    assert(_minMip < _sparseInfo.maxSparseLevel);
    assert(_minMip < _maxMip);
    assert(_transferrable);
    stripToMip(_minMip + 1);
}

void GL45Backend::derezTextures() const {
    if (GLTexture::getMemoryPressure() < 1.0f) {
        return;
    }

    Lock lock(texturesByMipCountsMutex);
    if (texturesByMipCounts.empty()) {
        qCDebug(gpugl45logging) << "No available textures to derez";
        return;
    }

    auto mipLevel = texturesByMipCounts.rbegin()->first;
    if (mipLevel <= 1) {
        qCDebug(gpugl45logging) << "Max mip levels " << mipLevel;
        return;
    }

    qCDebug(gpugl45logging) << "Allowed texture memory " << Texture::getAllowedGPUMemoryUsage();
    qCDebug(gpugl45logging) << "Used texture memory " << Context::getTextureGPUMemoryUsage();

    GL45Texture* targetTexture = nullptr;
    {
        auto& textures = texturesByMipCounts[mipLevel];
        assert(!textures.empty());
        targetTexture = *textures.begin();
    }
    lock.unlock();
    targetTexture->derez();
    qCDebug(gpugl45logging) << "New Used texture memory " << Context::getTextureGPUMemoryUsage();
}
