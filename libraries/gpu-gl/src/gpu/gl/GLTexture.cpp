//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLTexture.h"

#include <QtCore/QThread>
#include <NumericalConstants.h>

#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;


const GLenum GLTexture::CUBE_FACE_LAYOUT[GLTexture::TEXTURE_CUBE_NUM_FACES] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

const GLenum GLTexture::WRAP_MODES[Sampler::NUM_WRAP_MODES] = {
    GL_REPEAT,                         // WRAP_REPEAT,
    GL_MIRRORED_REPEAT,                // WRAP_MIRROR,
    GL_CLAMP_TO_EDGE,                  // WRAP_CLAMP,
    GL_CLAMP_TO_BORDER,                // WRAP_BORDER,
    GL_MIRROR_CLAMP_TO_EDGE_EXT        // WRAP_MIRROR_ONCE,
};     

const GLFilterMode GLTexture::FILTER_MODES[Sampler::NUM_FILTERS] = {
    { GL_NEAREST, GL_NEAREST },  //FILTER_MIN_MAG_POINT,
    { GL_NEAREST, GL_LINEAR },  //FILTER_MIN_POINT_MAG_LINEAR,
    { GL_LINEAR, GL_NEAREST },  //FILTER_MIN_LINEAR_MAG_POINT,
    { GL_LINEAR, GL_LINEAR },  //FILTER_MIN_MAG_LINEAR,

    { GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },  //FILTER_MIN_MAG_MIP_POINT,
    { GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST },  //FILTER_MIN_MAG_POINT_MIP_LINEAR,
    { GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR },  //FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
    { GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR },  //FILTER_MIN_POINT_MAG_MIP_LINEAR,
    { GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST },  //FILTER_MIN_LINEAR_MAG_MIP_POINT,
    { GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST },  //FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    { GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR },  //FILTER_MIN_MAG_LINEAR_MIP_POINT,
    { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR },  //FILTER_MIN_MAG_MIP_LINEAR,
    { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR }  //FILTER_ANISOTROPIC,
};

GLenum GLTexture::getGLTextureType(const Texture& texture) {
    switch (texture.getType()) {
    case Texture::TEX_2D:
        return GL_TEXTURE_2D;
        break;

    case Texture::TEX_CUBE:
        return GL_TEXTURE_CUBE_MAP;
        break;

    default:
        qFatal("Unsupported texture type");
    }
    Q_UNREACHABLE();
    return GL_TEXTURE_2D;
}


uint8_t GLTexture::getFaceCount(GLenum target) {
    switch (target) {
        case GL_TEXTURE_2D:
            return TEXTURE_2D_NUM_FACES;
        case GL_TEXTURE_CUBE_MAP:
            return TEXTURE_CUBE_NUM_FACES;
        default:
            Q_UNREACHABLE();
            break;
    }
}
const std::vector<GLenum>& GLTexture::getFaceTargets(GLenum target) {
    static std::vector<GLenum> cubeFaceTargets {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    static std::vector<GLenum> faceTargets {
        GL_TEXTURE_2D
    };
    switch (target) {
    case GL_TEXTURE_2D:
        return faceTargets;
    case GL_TEXTURE_CUBE_MAP:
        return cubeFaceTargets;
    default:
        Q_UNREACHABLE();
        break;
    }
    Q_UNREACHABLE();
    return faceTargets;
}

GLTexture::GLTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint id) :
    GLObject(backend, texture, id),
    _source(texture.source()),
    _target(getGLTextureType(texture)),
    _texelFormat(GLTexelFormat::evalGLTexelFormatInternal(texture.getTexelFormat()))
{
    Backend::setGPUObject(texture, this);
}

GLTexture::~GLTexture() {
    auto backend = _backend.lock();
    if (backend && _id) {
        backend->releaseTexture(_id, 0);
    }
}

Size GLTexture::copyMipFaceFromTexture(uint16_t sourceMip, uint16_t targetMip, uint8_t face) const {
    if (!_gpuObject.isStoredMipFaceAvailable(sourceMip)) {
        return 0;
    }
    auto dim = _gpuObject.evalMipDimensions(sourceMip);
    auto mipData = _gpuObject.accessStoredMipFace(sourceMip, face);
    auto mipSize = _gpuObject.getStoredMipFaceSize(sourceMip, face);
    if (mipData) {
        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), _gpuObject.getStoredMipFormat());
        return copyMipFaceLinesFromTexture(targetMip, face, dim, 0, texelFormat.internalFormat, texelFormat.format, texelFormat.type, mipSize, mipData->readData());
    } else {
        qCDebug(gpugllogging) << "Missing mipData level=" << sourceMip << " face=" << (int)face << " for texture " << _gpuObject.source().c_str();
    }
    return 0;
}


GLExternalTexture::GLExternalTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint id) 
    : Parent(backend, texture, id) {
    Backend::textureExternalCount.increment();
}

GLExternalTexture::~GLExternalTexture() {
    auto backend = _backend.lock();
    if (backend) {
        auto recycler = _gpuObject.getExternalRecycler();
        if (recycler) {
            backend->releaseExternalTexture(_id, recycler);
        } else {
            qCWarning(gpugllogging) << "No recycler available for texture " << _id << " possible leak";
        }
        const_cast<GLuint&>(_id) = 0;
    }
    Backend::textureExternalCount.decrement();
}


// Variable sized textures
using MemoryPressureState = GLVariableAllocationSupport::MemoryPressureState;
using WorkQueue = GLVariableAllocationSupport::WorkQueue;
using TransferJobPointer = GLVariableAllocationSupport::TransferJobPointer;

std::list<TextureWeakPointer> GLVariableAllocationSupport::_memoryManagedTextures;
MemoryPressureState GLVariableAllocationSupport::_memoryPressureState { MemoryPressureState::Idle };
std::atomic<bool> GLVariableAllocationSupport::_memoryPressureStateStale { false };
const uvec3 GLVariableAllocationSupport::INITIAL_MIP_TRANSFER_DIMENSIONS { 64, 64, 1 };
WorkQueue GLVariableAllocationSupport::_transferQueue;
WorkQueue GLVariableAllocationSupport::_promoteQueue;
WorkQueue GLVariableAllocationSupport::_demoteQueue;
size_t GLVariableAllocationSupport::_frameTexturesCreated { 0 };

#define OVERSUBSCRIBED_PRESSURE_VALUE 0.95f
#define UNDERSUBSCRIBED_PRESSURE_VALUE 0.85f
#define DEFAULT_ALLOWED_TEXTURE_MEMORY_MB ((size_t)1024)

static const size_t DEFAULT_ALLOWED_TEXTURE_MEMORY = MB_TO_BYTES(DEFAULT_ALLOWED_TEXTURE_MEMORY_MB);

using TransferJob = GLVariableAllocationSupport::TransferJob;

const uvec3 GLVariableAllocationSupport::MAX_TRANSFER_DIMENSIONS { 1024, 1024, 1 };
const size_t GLVariableAllocationSupport::MAX_TRANSFER_SIZE = GLVariableAllocationSupport::MAX_TRANSFER_DIMENSIONS.x * GLVariableAllocationSupport::MAX_TRANSFER_DIMENSIONS.y * 4;

#if THREADED_TEXTURE_BUFFERING

TexturePointer GLVariableAllocationSupport::_currentTransferTexture;
TransferJobPointer GLVariableAllocationSupport::_currentTransferJob;
QThreadPool* TransferJob::_bufferThreadPool { nullptr };

void TransferJob::startBufferingThread() {
    static std::once_flag once;
    std::call_once(once, [&] {
        _bufferThreadPool = new QThreadPool(qApp);
        _bufferThreadPool->setMaxThreadCount(1);
    });
}

#endif

TransferJob::TransferJob(const GLTexture& parent, uint16_t sourceMip, uint16_t targetMip, uint8_t face, uint32_t lines, uint32_t lineOffset)
    : _parent(parent) {

    auto transferDimensions = _parent._gpuObject.evalMipDimensions(sourceMip);
    GLenum format;
    GLenum internalFormat;
    GLenum type;
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_parent._gpuObject.getTexelFormat(), _parent._gpuObject.getStoredMipFormat());
    format = texelFormat.format;
    internalFormat = texelFormat.internalFormat;
    type = texelFormat.type;
    _transferSize = _parent._gpuObject.getStoredMipFaceSize(sourceMip, face);

    // If we're copying a subsection of the mip, do additional calculations to find the size and offset of the segment
    if (0 != lines) {
        transferDimensions.y = lines;
        auto dimensions = _parent._gpuObject.evalMipDimensions(sourceMip);
        auto bytesPerLine = (uint32_t)_transferSize / dimensions.y;
        _transferOffset = bytesPerLine * lineOffset;
        _transferSize = bytesPerLine * lines;
    }

    Backend::texturePendingGPUTransferMemSize.update(0, _transferSize);

    if (_transferSize > GLVariableAllocationSupport::MAX_TRANSFER_SIZE) {
        qCWarning(gpugllogging) << "Transfer size of " << _transferSize << " exceeds theoretical maximum transfer size";
    }

    // Buffering can invoke disk IO, so it should be off of the main and render threads
    _bufferingLambda = [=] {
        auto mipStorage = _parent._gpuObject.accessStoredMipFace(sourceMip, face);
        if (mipStorage) {
            _mipData = mipStorage->createView(_transferSize, _transferOffset);
        } else {
            qCWarning(gpugllogging) << "Buffering failed because mip could not be retrieved from texture " << _parent._source.c_str() ;
        }
    };

    _transferLambda = [=] {
        if (_mipData) {
            _parent.copyMipFaceLinesFromTexture(targetMip, face, transferDimensions, lineOffset, internalFormat, format, type, _mipData->size(), _mipData->readData());
            _mipData.reset();
        } else {
            qCWarning(gpugllogging) << "Transfer failed because mip could not be retrieved from texture " << _parent._source.c_str();
        }
    };
}

TransferJob::TransferJob(const GLTexture& parent, std::function<void()> transferLambda)
    : _parent(parent), _bufferingRequired(false), _transferLambda(transferLambda) {
}

TransferJob::~TransferJob() {
    Backend::texturePendingGPUTransferMemSize.update(_transferSize, 0);
}

bool TransferJob::tryTransfer() {
#if THREADED_TEXTURE_BUFFERING
    // Are we ready to transfer
    if (!bufferingCompleted()) {
        startBuffering();
        return false;
    }
#else
    if (_bufferingRequired) {
        _bufferingLambda();
    }
#endif
    _transferLambda();
    return true;
}

#if THREADED_TEXTURE_BUFFERING
bool TransferJob::bufferingRequired() const {
    if (!_bufferingRequired) {
        return false;
    }

    // The default state of a QFuture is with status Canceled | Started  | Finished, 
    // so we have to check isCancelled before we check the actual state
    if (_bufferingStatus.isCanceled()) {
        return true;
    }

    return !_bufferingStatus.isStarted();
}

bool TransferJob::bufferingCompleted() const {
    if (!_bufferingRequired) {
        return true;
    }

    // The default state of a QFuture is with status Canceled | Started  | Finished, 
    // so we have to check isCancelled before we check the actual state
    if (_bufferingStatus.isCanceled()) {
        return false;
    }

    return _bufferingStatus.isFinished();
}

void TransferJob::startBuffering() {
    if (bufferingRequired()) {
        assert(_bufferingStatus.isCanceled());
        _bufferingStatus = QtConcurrent::run(_bufferThreadPool, [=] {
            _bufferingLambda();
        });
        assert(!_bufferingStatus.isCanceled());
        assert(_bufferingStatus.isStarted());
    }
}
#endif

GLVariableAllocationSupport::GLVariableAllocationSupport() {
    _memoryPressureStateStale = true;
}

GLVariableAllocationSupport::~GLVariableAllocationSupport() {
    _memoryPressureStateStale = true;
}

void GLVariableAllocationSupport::addMemoryManagedTexture(const TexturePointer& texturePointer) {
    _memoryManagedTextures.push_back(texturePointer);
    if (MemoryPressureState::Idle != _memoryPressureState) {
        addToWorkQueue(texturePointer);
    }
}

void GLVariableAllocationSupport::addToWorkQueue(const TexturePointer& texturePointer) {
    GLTexture* gltexture = Backend::getGPUObject<GLTexture>(*texturePointer);
    GLVariableAllocationSupport* vargltexture = dynamic_cast<GLVariableAllocationSupport*>(gltexture);
    switch (_memoryPressureState) {
        case MemoryPressureState::Oversubscribed:
            if (vargltexture->canDemote()) {
                // Demote largest first
                _demoteQueue.push({ texturePointer, (float)gltexture->size() });
            }
            break;

        case MemoryPressureState::Undersubscribed:
            if (vargltexture->canPromote()) {
                // Promote smallest first
                _promoteQueue.push({ texturePointer, 1.0f / (float)gltexture->size() });
            }
            break;

        case MemoryPressureState::Transfer:
            if (vargltexture->hasPendingTransfers()) {
                // Transfer priority given to smaller mips first
                _transferQueue.push({ texturePointer, 1.0f / (float)gltexture->_gpuObject.evalMipSize(vargltexture->_populatedMip) });
            }
            break;

        case MemoryPressureState::Idle:
            Q_UNREACHABLE();
            break;
    }
}

WorkQueue& GLVariableAllocationSupport::getActiveWorkQueue() {
    static WorkQueue empty;
    switch (_memoryPressureState) {
        case MemoryPressureState::Oversubscribed:
            return _demoteQueue;

        case MemoryPressureState::Undersubscribed:
            return _promoteQueue;

        case MemoryPressureState::Transfer:
            return _transferQueue;

        case MemoryPressureState::Idle:
            Q_UNREACHABLE();
            break;
    }
    return empty;
}

// FIXME hack for stats display
QString getTextureMemoryPressureModeString() {
    switch (GLVariableAllocationSupport::_memoryPressureState) {
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

void GLVariableAllocationSupport::updateMemoryPressure() {
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
        GLTexture* gltexture = Backend::getGPUObject<GLTexture>(*texture);
        GLVariableAllocationSupport* vartexture = dynamic_cast<GLVariableAllocationSupport*>(gltexture);
        // Track how much the texture thinks it should be using
        idealMemoryAllocation += texture->evalTotalSize();
        // Track how much we're actually using
        totalVariableMemoryAllocation += gltexture->size();
        canDemote |= vartexture->canDemote();
        canPromote |= vartexture->canPromote();
        hasTransfers |= vartexture->hasPendingTransfers();
    }

    size_t unallocated = idealMemoryAllocation - totalVariableMemoryAllocation;
    float pressure = (float)totalVariableMemoryAllocation / (float)allowedMemoryAllocation;

    auto newState = MemoryPressureState::Idle;
    if (pressure < UNDERSUBSCRIBED_PRESSURE_VALUE && (unallocated != 0 && canPromote)) {
        newState = MemoryPressureState::Undersubscribed;
    } else if (pressure > OVERSUBSCRIBED_PRESSURE_VALUE && canDemote) {
        newState = MemoryPressureState::Oversubscribed;
    } else if (hasTransfers) {
        newState = MemoryPressureState::Transfer;
    }

    if (newState != _memoryPressureState) {
        _memoryPressureState = newState;
        // Clear the existing queue
        _transferQueue = WorkQueue();
        _promoteQueue = WorkQueue();
        _demoteQueue = WorkQueue();

        // Populate the existing textures into the queue
        if (_memoryPressureState != MemoryPressureState::Idle) {
            for (const auto& texture : strongTextures) {
                // Race conditions can still leave nulls in the list, so we need to check
                if (!texture) {
                    continue;
                }
                addToWorkQueue(texture);
            }
        }
    }
}

TexturePointer GLVariableAllocationSupport::getNextWorkQueueItem(WorkQueue& workQueue) {
    while (!workQueue.empty()) {
        auto workTarget = workQueue.top();

        auto texture = workTarget.first.lock();
        if (!texture) {
            workQueue.pop();
            continue;
        }

        // Check whether the resulting texture can actually have work performed
        GLTexture* gltexture = Backend::getGPUObject<GLTexture>(*texture);
        GLVariableAllocationSupport* vartexture = dynamic_cast<GLVariableAllocationSupport*>(gltexture);
        switch (_memoryPressureState) {
            case MemoryPressureState::Oversubscribed:
                if (vartexture->canDemote()) {
                    return texture;
                }
                break;

            case MemoryPressureState::Undersubscribed:
                if (vartexture->canPromote()) {
                    return texture;
                }
                break;

            case MemoryPressureState::Transfer:
                if (vartexture->hasPendingTransfers()) {
                    return texture;
                }
                break;

            case MemoryPressureState::Idle:
                Q_UNREACHABLE();
                break;
        }

        // If we got here, then the texture has no work to do in the current state, 
        // so pop it off the queue and continue
        workQueue.pop();
    }

    return TexturePointer();
}

void GLVariableAllocationSupport::processWorkQueue(WorkQueue& workQueue) {
    if (workQueue.empty()) {
        return;
    }

    // Get the front of the work queue to perform work
    auto texture = getNextWorkQueueItem(workQueue);
    if (!texture) {
        return;
    }

    // Grab the first item off the demote queue
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);

    GLTexture* gltexture = Backend::getGPUObject<GLTexture>(*texture);
    GLVariableAllocationSupport* vartexture = dynamic_cast<GLVariableAllocationSupport*>(gltexture);
    switch (_memoryPressureState) {
        case MemoryPressureState::Oversubscribed:
            vartexture->demote();
            workQueue.pop();
            addToWorkQueue(texture);
            _memoryPressureStateStale = true;
            break;

        case MemoryPressureState::Undersubscribed:
            vartexture->promote();
            workQueue.pop();
            addToWorkQueue(texture);
            _memoryPressureStateStale = true;
            break;

        case MemoryPressureState::Transfer:
            if (vartexture->executeNextTransfer(texture)) {
                workQueue.pop();
                addToWorkQueue(texture);

#if THREADED_TEXTURE_BUFFERING
                // Eagerly start the next buffering job if possible
                texture = getNextWorkQueueItem(workQueue);
                if (texture) {
                    gltexture = Backend::getGPUObject<GLTexture>(*texture);
                    vartexture = dynamic_cast<GLVariableAllocationSupport*>(gltexture);
                    vartexture->executeNextBuffer(texture);
                }
#endif
            }
            break;

        case MemoryPressureState::Idle:
            Q_UNREACHABLE();
            break;
    }
}

void GLVariableAllocationSupport::processWorkQueues() {
    if (MemoryPressureState::Idle == _memoryPressureState) {
        return;
    }

    auto& workQueue = getActiveWorkQueue();
    // Do work on the front of the queue
    processWorkQueue(workQueue);

    if (workQueue.empty()) {
        _memoryPressureState = MemoryPressureState::Idle;
        _memoryPressureStateStale = true;
    }
}

void GLVariableAllocationSupport::manageMemory() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    updateMemoryPressure();
    processWorkQueues();
}

bool GLVariableAllocationSupport::executeNextTransfer(const TexturePointer& currentTexture) {
#if THREADED_TEXTURE_BUFFERING
    // If a transfer job is active on the buffering thread, but has not completed it's buffering lambda,
    // then we need to exit early, since we don't want to have the transfer job leave scope while it's 
    // being used in another thread -- See https://highfidelity.fogbugz.com/f/cases/4626
    if (_currentTransferJob && !_currentTransferJob->bufferingCompleted()) {
        return false;
    }
#endif

    if (_populatedMip <= _allocatedMip) {
#if THREADED_TEXTURE_BUFFERING
        _currentTransferJob.reset();
        _currentTransferTexture.reset();
#endif
        return true;
    }

    // If the transfer queue is empty, rebuild it
    if (_pendingTransfers.empty()) {
        populateTransferQueue();
    }

    bool result = false;
    if (!_pendingTransfers.empty()) {
#if THREADED_TEXTURE_BUFFERING
        // If there is a current transfer, but it's not the top of the pending transfer queue, then it's an orphan, so we want to abandon it.
        if (_currentTransferJob && _currentTransferJob != _pendingTransfers.front()) {
            _currentTransferJob.reset();
        }

        if (!_currentTransferJob) {
            // Keeping hold of a strong pointer to the transfer job ensures that if the pending transfer queue is rebuilt, the transfer job
            // doesn't leave scope, causing a crash in the buffering thread
            _currentTransferJob = _pendingTransfers.front();

            // Keeping hold of a strong pointer during the transfer ensures that the transfer thread cannot try to access a destroyed texture
            _currentTransferTexture = currentTexture;
        }

        // transfer jobs use asynchronous buffering of the texture data because it may involve disk IO, so we execute a try here to determine if the buffering 
        // is complete
        if (_currentTransferJob->tryTransfer()) {
            _pendingTransfers.pop();
            // Once a given job is finished, release the shared pointers keeping them alive
            _currentTransferTexture.reset();
            _currentTransferJob.reset();
            result = true;
        }
#else
        if (_pendingTransfers.front()->tryTransfer()) {
            _pendingTransfers.pop();
            result = true;
        }
#endif
    }
    return result;
}

#if THREADED_TEXTURE_BUFFERING
void GLVariableAllocationSupport::executeNextBuffer(const TexturePointer& currentTexture) {
    if (_currentTransferJob && !_currentTransferJob->bufferingCompleted()) {
        return;
    }

    // If the transfer queue is empty, rebuild it
    if (_pendingTransfers.empty()) {
        populateTransferQueue();
    }

    if (!_pendingTransfers.empty()) {
        if (!_currentTransferJob) {
            _currentTransferJob = _pendingTransfers.front();
            _currentTransferTexture = currentTexture;
        }

        _currentTransferJob->startBuffering();
    }
}
#endif

void GLVariableAllocationSupport::incrementPopulatedSize(Size delta) const {
    _populatedSize += delta;
    // Keep the 2 code paths to be able to debug
    if (_size < _populatedSize) {
        Backend::textureResourcePopulatedGPUMemSize.update(0, delta);
    } else  {
        Backend::textureResourcePopulatedGPUMemSize.update(0, delta);
    }
}
void GLVariableAllocationSupport::decrementPopulatedSize(Size delta) const {
    _populatedSize -= delta;
    // Keep the 2 code paths to be able to debug
    if (_size < _populatedSize) {
        Backend::textureResourcePopulatedGPUMemSize.update(delta, 0);
    } else  {
        Backend::textureResourcePopulatedGPUMemSize.update(delta, 0);
    }
}


