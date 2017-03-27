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

// Variable sized textures
using GL45VariableAllocationTexture = GL45Backend::GL45VariableAllocationTexture;
using MemoryPressureState = GL45VariableAllocationTexture::MemoryPressureState;
using WorkQueue = GL45VariableAllocationTexture::WorkQueue;

std::list<TextureWeakPointer> GL45VariableAllocationTexture::_memoryManagedTextures;
MemoryPressureState GL45VariableAllocationTexture::_memoryPressureState = MemoryPressureState::Idle;
std::atomic<bool> GL45VariableAllocationTexture::_memoryPressureStateStale { false };
const uvec3 GL45VariableAllocationTexture::INITIAL_MIP_TRANSFER_DIMENSIONS { 64, 64, 1 };
WorkQueue GL45VariableAllocationTexture::_transferQueue;
WorkQueue GL45VariableAllocationTexture::_promoteQueue;
WorkQueue GL45VariableAllocationTexture::_demoteQueue;
TexturePointer GL45VariableAllocationTexture::_currentTransferTexture;

#define OVERSUBSCRIBED_PRESSURE_VALUE 0.95f
#define UNDERSUBSCRIBED_PRESSURE_VALUE 0.85f
#define DEFAULT_ALLOWED_TEXTURE_MEMORY_MB ((size_t)1024)

static const size_t DEFAULT_ALLOWED_TEXTURE_MEMORY = MB_TO_BYTES(DEFAULT_ALLOWED_TEXTURE_MEMORY_MB);

using TransferJob = GL45VariableAllocationTexture::TransferJob;

static const uvec3 MAX_TRANSFER_DIMENSIONS { 1024, 1024, 1 };
static const size_t MAX_TRANSFER_SIZE = MAX_TRANSFER_DIMENSIONS.x * MAX_TRANSFER_DIMENSIONS.y * 4;

#if THREADED_TEXTURE_BUFFERING
std::shared_ptr<std::thread> TransferJob::_bufferThread { nullptr };
std::atomic<bool> TransferJob::_shutdownBufferingThread { false };
Mutex TransferJob::_mutex;
TransferJob::VoidLambdaQueue TransferJob::_bufferLambdaQueue;

void TransferJob::startTransferLoop() {
    if (_bufferThread) {
        return;
    }
    _shutdownBufferingThread = false;
    _bufferThread = std::make_shared<std::thread>([] {
        TransferJob::bufferLoop();
    });
}

void TransferJob::stopTransferLoop() {
    if (!_bufferThread) {
        return;
    }
    _shutdownBufferingThread = true;
    _bufferThread->join();
    _bufferThread.reset();
    _shutdownBufferingThread = false;
}
#endif

TransferJob::TransferJob(const GL45VariableAllocationTexture& parent, uint16_t sourceMip, uint16_t targetMip, uint8_t face, uint32_t lines, uint32_t lineOffset)
    : _parent(parent) {

    auto transferDimensions = _parent._gpuObject.evalMipDimensions(sourceMip);
    GLenum format;
    GLenum type;
    auto mipData = _parent._gpuObject.accessStoredMipFace(sourceMip, face);
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_parent._gpuObject.getTexelFormat(), _parent._gpuObject.getStoredMipFormat());
    format = texelFormat.format;
    type = texelFormat.type;

    if (0 == lines) {
        _transferSize = mipData->getSize();
        _bufferingLambda = [=] {
            _buffer.resize(_transferSize);
            memcpy(&_buffer[0], mipData->readData(), _transferSize);
            _bufferingCompleted = true;
        };

    } else {
        transferDimensions.y = lines;
        auto dimensions = _parent._gpuObject.evalMipDimensions(sourceMip);
        auto mipSize = mipData->getSize();
        auto bytesPerLine = (uint32_t)mipSize / dimensions.y;
        _transferSize = bytesPerLine * lines;
        auto sourceOffset = bytesPerLine * lineOffset;
        _bufferingLambda = [=] {
            _buffer.resize(_transferSize);
            memcpy(&_buffer[0], mipData->readData() + sourceOffset, _transferSize);
            _bufferingCompleted = true;
        };
    }

    Backend::updateTextureTransferPendingSize(0, _transferSize);

    _transferLambda = [=] {
        _parent.copyMipFaceLinesFromTexture(targetMip, face, transferDimensions, lineOffset, format, type, _buffer.data());
        std::vector<uint8_t> emptyVector;
        _buffer.swap(emptyVector);
    };
}

TransferJob::TransferJob(const GL45VariableAllocationTexture& parent, std::function<void()> transferLambda)
    : _parent(parent), _bufferingCompleted(true), _transferLambda(transferLambda) {
}

TransferJob::~TransferJob() {
    Backend::updateTextureTransferPendingSize(_transferSize, 0);
}


bool TransferJob::tryTransfer() {
    // Disable threaded texture transfer for now
#if THREADED_TEXTURE_BUFFERING
    // Are we ready to transfer
    if (_bufferingCompleted) {
        _transferLambda();
        return true;
    }

    startBuffering();
    return false;
#else
    if (!_bufferingCompleted) {
        _bufferingLambda();
        _bufferingCompleted = true;
    }
    _transferLambda();
    return true;
#endif
}

#if THREADED_TEXTURE_BUFFERING

void TransferJob::startBuffering() {
    if (_bufferingStarted) {
        return;
    }
    _bufferingStarted = true;
    {
        Lock lock(_mutex);
        _bufferLambdaQueue.push(_bufferingLambda);
    }
}

void TransferJob::bufferLoop() {
    while (!_shutdownBufferingThread) {
        VoidLambdaQueue workingQueue;
        {
            Lock lock(_mutex);
            _bufferLambdaQueue.swap(workingQueue);
        }

        if (workingQueue.empty()) {
            QThread::msleep(5);
            continue;
        }

        while (!workingQueue.empty()) {
            workingQueue.front()();
            workingQueue.pop();
        }
    }
}
#endif


void GL45VariableAllocationTexture::addMemoryManagedTexture(const TexturePointer& texturePointer) {
    _memoryManagedTextures.push_back(texturePointer);
    addToWorkQueue(texturePointer);
}

void GL45VariableAllocationTexture::addToWorkQueue(const TexturePointer& texturePointer) {
    GL45VariableAllocationTexture* object = Backend::getGPUObject<GL45VariableAllocationTexture>(*texturePointer);
    switch (_memoryPressureState) {
        case MemoryPressureState::Oversubscribed:
            if (object->canDemote()) {
                // Demote largest first
                _demoteQueue.push({ texturePointer, (float)object->size() });
            }
            break;

        case MemoryPressureState::Undersubscribed:
            if (object->canPromote()) {
                // Promote smallest first
                _promoteQueue.push({ texturePointer, 1.0f / (float)object->size() });
            }
            break;

        case MemoryPressureState::Transfer:
            if (object->hasPendingTransfers()) {
                // Transfer priority given to smaller mips first
                _transferQueue.push({ texturePointer, 1.0f / (float)object->_gpuObject.evalMipSize(object->_populatedMip) });
            }
            break;

        case MemoryPressureState::Idle:
            break;

        default:
            Q_UNREACHABLE();
    }
}

WorkQueue& GL45VariableAllocationTexture::getActiveWorkQueue() {
    static WorkQueue empty;
    switch (_memoryPressureState) {
        case MemoryPressureState::Oversubscribed:
            return _demoteQueue;

        case MemoryPressureState::Undersubscribed:
            return _promoteQueue;

        case MemoryPressureState::Transfer:
            return _transferQueue;

        default:
            break;
    }
    Q_UNREACHABLE();
    return empty;
}

// FIXME hack for stats display
QString getTextureMemoryPressureModeString() {
    switch (GL45VariableAllocationTexture::_memoryPressureState) {
        case MemoryPressureState::Oversubscribed:
            return "Oversubscribed";

        case MemoryPressureState::Undersubscribed:
            return "Undersubscribed";

        case MemoryPressureState::Transfer:
            return "Transfer";

        case MemoryPressureState::Idle:
            return "Idle";
    }
    Q_UNREACHABLE();
    return "Unknown";
}

void GL45VariableAllocationTexture::updateMemoryPressure() {
    static size_t lastAllowedMemoryAllocation = gpu::Texture::getAllowedGPUMemoryUsage();

    size_t allowedMemoryAllocation = gpu::Texture::getAllowedGPUMemoryUsage();
    if (0 == allowedMemoryAllocation) {
        allowedMemoryAllocation = DEFAULT_ALLOWED_TEXTURE_MEMORY;
    }

    // If the user explicitly changed the allowed memory usage, we need to mark ourselves stale 
    // so that we react
    if (allowedMemoryAllocation != lastAllowedMemoryAllocation) {
        _memoryPressureStateStale = true;
        lastAllowedMemoryAllocation = allowedMemoryAllocation;
    }

    if (!_memoryPressureStateStale.exchange(false)) {
        return;
    }

    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);

    // Clear any defunct textures (weak pointers that no longer have a valid texture)
    _memoryManagedTextures.remove_if([&](const TextureWeakPointer& weakPointer) { 
        return weakPointer.expired(); 
    });

    // Convert weak pointers to strong.  This new list may still contain nulls if a texture was 
    // deleted on another thread between the previous line and this one
    std::vector<TexturePointer> strongTextures; {
        strongTextures.reserve(_memoryManagedTextures.size());
        std::transform(
            _memoryManagedTextures.begin(), _memoryManagedTextures.end(),
            std::back_inserter(strongTextures),
            [](const TextureWeakPointer& p) { return p.lock(); });
    }

    size_t totalVariableMemoryAllocation = 0;
    size_t idealMemoryAllocation = 0;
    bool canDemote = false;
    bool canPromote = false;
    bool hasTransfers = false;
    for (const auto& texture : strongTextures) {
        // Race conditions can still leave nulls in the list, so we need to check
        if (!texture) {
            continue;
        }
        GL45VariableAllocationTexture* object = Backend::getGPUObject<GL45VariableAllocationTexture>(*texture);
        // Track how much the texture thinks it should be using
        idealMemoryAllocation += texture->evalTotalSize();
        // Track how much we're actually using
        totalVariableMemoryAllocation += object->size();
        canDemote |= object->canDemote();
        canPromote |= object->canPromote();
        hasTransfers |= object->hasPendingTransfers();
    }

    size_t unallocated = idealMemoryAllocation - totalVariableMemoryAllocation;
    float pressure = (float)totalVariableMemoryAllocation / (float)allowedMemoryAllocation;

    auto newState = MemoryPressureState::Idle;
    if (pressure > OVERSUBSCRIBED_PRESSURE_VALUE && canDemote) {
        newState = MemoryPressureState::Oversubscribed;
    } else if (pressure < UNDERSUBSCRIBED_PRESSURE_VALUE && unallocated != 0 && canPromote) {
        newState = MemoryPressureState::Undersubscribed;
    } else if (hasTransfers) {
        newState = MemoryPressureState::Transfer;
    }

    if (newState != _memoryPressureState) {
#if THREADED_TEXTURE_BUFFERING
        if (MemoryPressureState::Transfer == _memoryPressureState) {
            TransferJob::stopTransferLoop();
        }
        _memoryPressureState = newState;
        if (MemoryPressureState::Transfer == _memoryPressureState) {
            TransferJob::startTransferLoop();
        }
#else
        _memoryPressureState = newState;
#endif
        // Clear the existing queue
        _transferQueue = WorkQueue();
        _promoteQueue = WorkQueue();
        _demoteQueue = WorkQueue();

        // Populate the existing textures into the queue
        for (const auto& texture : strongTextures) {
            addToWorkQueue(texture);
        }
    }
}

void GL45VariableAllocationTexture::processWorkQueues() {
    if (MemoryPressureState::Idle == _memoryPressureState) {
        return;
    }

    auto& workQueue = getActiveWorkQueue();
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    while (!workQueue.empty()) {
        auto workTarget = workQueue.top();
        workQueue.pop();
        auto texture = workTarget.first.lock();
        if (!texture) {
            continue;
        }

        // Grab the first item off the demote queue
        GL45VariableAllocationTexture* object = Backend::getGPUObject<GL45VariableAllocationTexture>(*texture);
        if (MemoryPressureState::Oversubscribed == _memoryPressureState) {
            if (!object->canDemote()) {
                continue;
            }
            object->demote();
        } else if (MemoryPressureState::Undersubscribed == _memoryPressureState) {
            if (!object->canPromote()) {
                continue;
            }
            object->promote();
        } else if (MemoryPressureState::Transfer == _memoryPressureState) {
            if (!object->hasPendingTransfers()) {
                continue;
            }
            object->executeNextTransfer(texture);
        } else {
            Q_UNREACHABLE();
        }

        // Reinject into the queue if more work to be done
        addToWorkQueue(texture);
        break;
    }

    if (workQueue.empty()) {
        _memoryPressureStateStale = true;
    }
}

void GL45VariableAllocationTexture::manageMemory() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    updateMemoryPressure();
    processWorkQueues();
}

size_t GL45VariableAllocationTexture::_frameTexturesCreated { 0 };

GL45VariableAllocationTexture::GL45VariableAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45Texture(backend, texture) {
    ++_frameTexturesCreated;
}

GL45VariableAllocationTexture::~GL45VariableAllocationTexture() {
    _memoryPressureStateStale = true;
    Backend::updateTextureGPUMemoryUsage(_size, 0);
}

void GL45VariableAllocationTexture::executeNextTransfer(const TexturePointer& currentTexture) {
    if (_populatedMip <= _allocatedMip) {
        return;
    }

    if (_pendingTransfers.empty()) {
        populateTransferQueue();
    }

    if (!_pendingTransfers.empty()) {
        // Keeping hold of a strong pointer during the transfer ensures that the transfer thread cannot try to access a destroyed texture
        _currentTransferTexture = currentTexture;
        if (_pendingTransfers.front()->tryTransfer()) {
            _pendingTransfers.pop();
            _currentTransferTexture.reset();
        }
    }
}

// Managed size resource textures
using GL45ResourceTexture = GL45Backend::GL45ResourceTexture;

GL45ResourceTexture::GL45ResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45VariableAllocationTexture(backend, texture) {
    auto mipLevels = texture.evalNumMips();
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
    copyMipsFromTexture();
    syncSampler();

}

void GL45ResourceTexture::allocateStorage(uint16 allocatedMip) {
    _allocatedMip = allocatedMip;
    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const auto dimensions = _gpuObject.evalMipDimensions(_allocatedMip);
    const auto totalMips = _gpuObject.getNumMipLevels();
    const auto mips = totalMips - _allocatedMip;
    glTextureStorage2D(_id, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);
    auto mipLevels = _gpuObject.getNumMipLevels();
    _size = 0;
    for (uint16_t mip = _allocatedMip; mip < mipLevels; ++mip) {
        _size += _gpuObject.evalMipSize(mip);
    }
    Backend::updateTextureGPUMemoryUsage(0, _size);

}

void GL45ResourceTexture::copyMipsFromTexture() {
    auto mipLevels = _gpuObject.getNumMipLevels();
    size_t maxFace = GLTexture::getFaceCount(_target);
    for (uint16_t sourceMip = _populatedMip; sourceMip < mipLevels; ++sourceMip) {
        uint16_t targetMip = sourceMip - _allocatedMip;
        for (uint8_t face = 0; face < maxFace; ++face) {
            copyMipFaceFromTexture(sourceMip, targetMip, face);
        }
    }
}

void GL45ResourceTexture::syncSampler() const {
    Parent::syncSampler();
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, _populatedMip - _allocatedMip);
}

void GL45ResourceTexture::promote() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    Q_ASSERT(_allocatedMip > 0);
    GLuint oldId = _id;
    uint32_t oldSize = _size;
    // create new texture
    const_cast<GLuint&>(_id) = allocate(_gpuObject);
    uint16_t oldAllocatedMip = _allocatedMip;
    // allocate storage for new level
    allocateStorage(_allocatedMip - std::min<uint16_t>(_allocatedMip, 2));
    uint16_t mips = _gpuObject.getNumMipLevels();
    // copy pre-existing mips
    for (uint16_t mip = _populatedMip; mip < mips; ++mip) {
        auto mipDimensions = _gpuObject.evalMipDimensions(mip);
        uint16_t targetMip = mip - _allocatedMip;
        uint16_t sourceMip = mip - oldAllocatedMip;
        auto faces = getFaceCount(_target);
        for (uint8_t face = 0; face < faces; ++face) {
            glCopyImageSubData(
                oldId, _target, sourceMip, 0, 0, face,
                _id, _target, targetMip, 0, 0, face,
                mipDimensions.x, mipDimensions.y, 1
                );
            (void)CHECK_GL_ERROR();
        }
    }
    // destroy the old texture
    glDeleteTextures(1, &oldId);
    // update the memory usage
    Backend::updateTextureGPUMemoryUsage(oldSize, 0);
    _memoryPressureStateStale = true;
    syncSampler();
    populateTransferQueue();
}

void GL45ResourceTexture::demote() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    Q_ASSERT(_allocatedMip < _maxAllocatedMip);
    auto oldId = _id;
    auto oldSize = _size;
    const_cast<GLuint&>(_id) = allocate(_gpuObject);
    allocateStorage(_allocatedMip + 1);
    _populatedMip = std::max(_populatedMip, _allocatedMip);
    uint16_t mips = _gpuObject.getNumMipLevels();
    // copy pre-existing mips
    for (uint16_t mip = _populatedMip; mip < mips; ++mip) {
        auto mipDimensions = _gpuObject.evalMipDimensions(mip);
        uint16_t targetMip = mip - _allocatedMip;
        uint16_t sourceMip = targetMip + 1;
        auto faces = getFaceCount(_target);
        for (uint8_t face = 0; face < faces; ++face) {
            glCopyImageSubData(
                oldId, _target, sourceMip, 0, 0, face,
                _id, _target, targetMip, 0, 0, face,
                mipDimensions.x, mipDimensions.y, 1
            );
            (void)CHECK_GL_ERROR();
        }
    }
    // destroy the old texture
    glDeleteTextures(1, &oldId);
    // update the memory usage
    Backend::updateTextureGPUMemoryUsage(oldSize, 0);
    _memoryPressureStateStale = true;
    syncSampler();
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
            auto mipData = _gpuObject.accessStoredMipFace(sourceMip, face);
            const auto lines = mipDimensions.y;
            auto bytesPerLine = (uint32_t)mipData->getSize() / lines;
            Q_ASSERT(0 == (mipData->getSize() % lines));
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
