//
//  GLBackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"

#include <unordered_set>
#include <unordered_map>
#include <QtCore/QThread>

#include "GLBackendShared.h"

#include "GLBackendTextureTransfer.h"

using namespace gpu;
using namespace gpu::gl;


GLenum gpuToGLTextureType(const Texture& texture) {
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

GLuint allocateSingleTexture() {
    Backend::incrementTextureGPUCount();
    GLuint result;
    glGenTextures(1, &result);
    return result;
}


// FIXME placeholder for texture memory over-use
#define DEFAULT_MAX_MEMORY_MB 256

float GLBackend::GLTexture::getMemoryPressure() {
    // Check for an explicit memory limit
    auto availableTextureMemory = Texture::getAllowedGPUMemoryUsage();

    // If no memory limit has been set, use a percentage of the total dedicated memory
    if (!availableTextureMemory) {
        auto totalGpuMemory = GLBackend::getDedicatedMemory();

        // If no limit has been explicitly set, and the dedicated memory can't be determined, 
        // just use a fallback fixed value of 256 MB
        if (!totalGpuMemory) {
            totalGpuMemory = MB_TO_BYTES(DEFAULT_MAX_MEMORY_MB);
        }

        // Allow 75% of all available GPU memory to be consumed by textures
        // FIXME overly conservative?
        availableTextureMemory = (totalGpuMemory >> 2) * 3;
    }

    // Return the consumed texture memory divided by the available texture memory.
    auto consumedGpuMemory = Context::getTextureGPUMemoryUsage();
    return (float)consumedGpuMemory / (float)availableTextureMemory;
}

GLBackend::GLTexture::DownsampleSource::DownsampleSource(GLTexture& oldTexture) :
    _texture(oldTexture._privateTexture),
    _minMip(oldTexture._minMip),
    _maxMip(oldTexture._maxMip) 
{
    // Take ownership of the GL texture
    oldTexture._texture = oldTexture._privateTexture = 0;
}

GLBackend::GLTexture::DownsampleSource::~DownsampleSource() {
    if (_texture) {
        Backend::decrementTextureGPUCount();
        glDeleteTextures(1, &_texture);
    }
}

const GLenum GLBackend::GLTexture::CUBE_FACE_LAYOUT[6] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

static std::map<uint16, size_t> _textureCountByMips;
static uint16 _currentMaxMipCount { 0 };

GLBackend::GLTexture::GLTexture(bool transferrable, const Texture& texture, bool init) :
    _storageStamp(texture.getStamp()),
    _target(gpuToGLTextureType(texture)),
    _maxMip(texture.maxMip()),
    _minMip(texture.minMip()),
    _transferrable(transferrable),
    _virtualSize(texture.evalTotalSize()),
    _size(_virtualSize),
    _gpuTexture(texture) 
{ 
    Q_UNUSED(init); 

    if (_transferrable) {
        uint16 mipCount = usedMipLevels();
        _currentMaxMipCount = std::max(_currentMaxMipCount, mipCount);
        if (!_textureCountByMips.count(mipCount)) {
            _textureCountByMips[mipCount] = 1;
        } else {
            ++_textureCountByMips[mipCount];
        }
    } else {
        withPreservedTexture([&] {
            createTexture();
        });
        _contentStamp = _gpuTexture.getDataStamp();
        postTransfer();
    }
    
    Backend::updateTextureGPUMemoryUsage(0, _size);
    Backend::updateTextureGPUVirtualMemoryUsage(0, _virtualSize);
}

// Create the texture and allocate storage
GLBackend::GLTexture::GLTexture(bool transferrable, const Texture& texture) : 
    GLTexture(transferrable, texture, true) 
{
    Backend::setGPUObject(texture, this);
}

// Create the texture and copy from the original higher resolution version
GLBackend::GLTexture::GLTexture(GLTexture& originalTexture, const gpu::Texture& texture) :
    GLTexture(originalTexture._transferrable, texture, true) 
{
    if (!originalTexture._texture) {
        qFatal("Invalid original texture");
    }
    Q_ASSERT(_minMip >= originalTexture._minMip);
    // Our downsampler takes ownership of the texture
    _downsampleSource = std::make_shared<DownsampleSource>(originalTexture);
    _texture = _downsampleSource->_texture;

    // Set the GPU object last because that implicitly destroys the originalTexture object
    Backend::setGPUObject(texture, this);
}

GLBackend::GLTexture::~GLTexture() {
    if (_privateTexture != 0) {
        Backend::decrementTextureGPUCount();
        glDeleteTextures(1, &_privateTexture);
    }

    if (_transferrable) {
        uint16 mipCount = usedMipLevels();
        Q_ASSERT(_textureCountByMips.count(mipCount));
        auto& numTexturesForMipCount = _textureCountByMips[mipCount];
        --numTexturesForMipCount;
        if (0 == numTexturesForMipCount) {
            _textureCountByMips.erase(mipCount);
            if (mipCount == _currentMaxMipCount) {
                _currentMaxMipCount = _textureCountByMips.rbegin()->first;
            }
        }
    }

    Backend::updateTextureGPUMemoryUsage(_size, 0);
    Backend::updateTextureGPUVirtualMemoryUsage(_virtualSize, 0);
}

const std::vector<GLenum>& GLBackend::GLTexture::getFaceTargets() const {
    static std::vector<GLenum> cubeFaceTargets {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    static std::vector<GLenum> faceTargets {
        GL_TEXTURE_2D
    };
    switch (_target) {
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

void GLBackend::GLTexture::withPreservedTexture(std::function<void()> f) {
    GLint boundTex = -1;
    switch (_target) {
    case GL_TEXTURE_2D:
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
        break;

    case GL_TEXTURE_CUBE_MAP:
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundTex);
        break;

    default:
        qFatal("Unsupported texture type");
    }
    (void)CHECK_GL_ERROR();

    f();

    glBindTexture(_target, boundTex);
    (void)CHECK_GL_ERROR();
}

void GLBackend::GLTexture::createTexture() {
    _privateTexture = allocateSingleTexture();

    glBindTexture(_target, _privateTexture);
    (void)CHECK_GL_ERROR();

    allocateStorage();
    (void)CHECK_GL_ERROR();

    syncSampler(_gpuTexture.getSampler(), _gpuTexture.getType(), this);
    (void)CHECK_GL_ERROR();
}

void GLBackend::GLTexture::allocateStorage() {
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuTexture.getTexelFormat());
    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, 0);
    (void)CHECK_GL_ERROR();
    glTexParameteri(_target, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    (void)CHECK_GL_ERROR();
    if (GLEW_VERSION_4_2 && !_gpuTexture.getTexelFormat().isCompressed()) {
        // Get the dimensions, accounting for the downgrade level
        Vec3u dimensions = _gpuTexture.evalMipDimensions(_minMip);
        glTexStorage2D(_target, usedMipLevels(), texelFormat.internalFormat, dimensions.x, dimensions.y);
        (void)CHECK_GL_ERROR();
    } else {
        for (uint16_t l = _minMip; l <= _maxMip; l++) {
            // Get the mip level dimensions, accounting for the downgrade level
            Vec3u dimensions = _gpuTexture.evalMipDimensions(l);
            for (GLenum target : getFaceTargets()) {
                glTexImage2D(target, l - _minMip, texelFormat.internalFormat, dimensions.x, dimensions.y, 0, texelFormat.format, texelFormat.type, NULL);
                (void)CHECK_GL_ERROR();
            }
        }
    }
}


void GLBackend::GLTexture::setSize(GLuint size) {
    Backend::updateTextureGPUMemoryUsage(_size, size);
    _size = size;
}

void GLBackend::GLTexture::updateSize() {
    setSize(_virtualSize);
    if (!_texture) {
        return;
    }

    if (_gpuTexture.getTexelFormat().isCompressed()) {
        GLenum proxyType = GL_TEXTURE_2D;
        GLuint numFaces = 1;
        if (_gpuTexture.getType() == gpu::Texture::TEX_CUBE) {
            proxyType = CUBE_FACE_LAYOUT[0];
            numFaces = CUBE_NUM_FACES;
        }
        GLint gpuSize{ 0 };
        glGetTexLevelParameteriv(proxyType, 0, GL_TEXTURE_COMPRESSED, &gpuSize);
        (void)CHECK_GL_ERROR();

        if (gpuSize) {
            for (GLuint level = _minMip; level < _maxMip; level++) {
                GLint levelSize{ 0 };
                glGetTexLevelParameteriv(proxyType, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &levelSize);
                levelSize *= numFaces;
                
                if (levelSize <= 0) {
                    break;
                }
                gpuSize += levelSize;
            }
            (void)CHECK_GL_ERROR();
            setSize(gpuSize);
            return;
        } 
    } 
}

bool GLBackend::GLTexture::isInvalid() const {
    return _storageStamp < _gpuTexture.getStamp();
}

bool GLBackend::GLTexture::isOutdated() const {
    return GLTexture::Idle == _syncState && _contentStamp < _gpuTexture.getDataStamp();
}

bool GLBackend::GLTexture::isOverMaxMemory() const {
    // FIXME switch to using the max mip count used from the previous frame
    if (usedMipLevels() < _currentMaxMipCount) {
        return false;
    }
    Q_ASSERT(usedMipLevels() == _currentMaxMipCount);

    if (getMemoryPressure() < 1.0f) {
        return false;
    }

    return true;
}

bool GLBackend::GLTexture::isReady() const {
    // If we have an invalid texture, we're never ready
    if (isInvalid()) {
        return false;
    }

    // If we're out of date, but the transfer is in progress, report ready
    // as a special case
    auto syncState = _syncState.load();

    if (isOutdated()) {
        return Idle != syncState;
    }

    if (Idle != syncState) {
        return false;
    }

    return true;
}

// Move content bits from the CPU to the GPU for a given mip / face
void GLBackend::GLTexture::transferMip(uint16_t mipLevel, uint8_t face) const {
    auto mip = _gpuTexture.accessStoredMipFace(mipLevel, face);
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuTexture.getTexelFormat(), mip->getFormat());
    //GLenum target = getFaceTargets()[face];
    GLenum target = _target == GL_TEXTURE_2D ? GL_TEXTURE_2D : CUBE_FACE_LAYOUT[face];
    auto size = _gpuTexture.evalMipDimensions(mipLevel);
    glTexSubImage2D(target, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
    (void)CHECK_GL_ERROR();
}

// This should never happen on the main thread
// Move content bits from the CPU to the GPU
void GLBackend::GLTexture::transfer() const {
    PROFILE_RANGE(__FUNCTION__);
    //qDebug() << "Transferring texture: " << _privateTexture;
    // Need to update the content of the GPU object from the source sysmem of the texture
    if (_contentStamp >= _gpuTexture.getDataStamp()) {
        return;
    }

    glBindTexture(_target, _privateTexture);
    (void)CHECK_GL_ERROR();

    if (_downsampleSource) {
        GLuint fbo { 0 };
        glGenFramebuffers(1, &fbo);
        (void)CHECK_GL_ERROR();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        (void)CHECK_GL_ERROR();
        // Find the distance between the old min mip and the new one
        uint16 mipOffset = _minMip - _downsampleSource->_minMip;
        for (uint16 i = _minMip; i <= _maxMip; ++i) {
            uint16 targetMip = i - _minMip;
            uint16 sourceMip = targetMip + mipOffset;
            Vec3u dimensions = _gpuTexture.evalMipDimensions(i);
            for (GLenum target : getFaceTargets()) {
                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _downsampleSource->_texture, sourceMip);
                (void)CHECK_GL_ERROR();
                glCopyTexSubImage2D(target, targetMip, 0, 0, 0, 0, dimensions.x, dimensions.y);
                (void)CHECK_GL_ERROR();
            }
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
    } else {
        // GO through the process of allocating the correct storage and/or update the content
        switch (_gpuTexture.getType()) {
        case Texture::TEX_2D: 
            {
                for (uint16_t i = _minMip; i <= _maxMip; ++i) {
                    if (_gpuTexture.isStoredMipFaceAvailable(i)) {
                        transferMip(i);
                    }
                }
            }
            break;

        case Texture::TEX_CUBE:
            // transfer pixels from each faces
            for (uint8_t f = 0; f < CUBE_NUM_FACES; f++) {
                for (uint16_t i = 0; i < Sampler::MAX_MIP_LEVEL; ++i) {
                    if (_gpuTexture.isStoredMipFaceAvailable(i, f)) {
                        transferMip(i, f);
                    }
                }
            }
            break;

        default:
            qCWarning(gpugllogging) << __FUNCTION__ << " case for Texture Type " << _gpuTexture.getType() << " not supported";
            break;
        }
    }
    if (_gpuTexture.isAutogenerateMips()) {
        glGenerateMipmap(_target);
        (void)CHECK_GL_ERROR();
    }
}

// Do any post-transfer operations that might be required on the main context / rendering thread
void GLBackend::GLTexture::postTransfer() {
    setSyncState(GLTexture::Idle);

    // The public gltexture becaomes available
    _texture = _privateTexture;

    _downsampleSource.reset();

    // At this point the mip pixels have been loaded, we can notify the gpu texture to abandon it's memory
    switch (_gpuTexture.getType()) {
        case Texture::TEX_2D:
            for (uint16_t i = 0; i < Sampler::MAX_MIP_LEVEL; ++i) {
                if (_gpuTexture.isStoredMipFaceAvailable(i)) {
                    _gpuTexture.notifyMipFaceGPULoaded(i);
                }
            }
            break;

        case Texture::TEX_CUBE:
            // transfer pixels from each faces
            for (uint8_t f = 0; f < CUBE_NUM_FACES; f++) {
                for (uint16_t i = 0; i < Sampler::MAX_MIP_LEVEL; ++i) {
                    if (_gpuTexture.isStoredMipFaceAvailable(i, f)) {
                        _gpuTexture.notifyMipFaceGPULoaded(i, f);
                    }
                }
            }
            break;

        default:
            qCWarning(gpugllogging) << __FUNCTION__ << " case for Texture Type " << _gpuTexture.getType() << " not supported";
            break;
    }
}

GLBackend::GLTexture* GLBackend::syncGPUObject(const TexturePointer& texturePointer, bool needTransfer) {
    const Texture& texture = *texturePointer;
    if (!texture.isDefined()) {
        // NO texture definition yet so let's avoid thinking
        return nullptr;
    }

    // If the object hasn't been created, or the object definition is out of date, drop and re-create
    GLTexture* object = Backend::getGPUObject<GLBackend::GLTexture>(texture);

    // Create the texture if need be (force re-creation if the storage stamp changes
    // for easier use of immutable storage)
    if (!object || object->isInvalid()) {
        // This automatically any previous texture
        object = new GLTexture(needTransfer, texture);
    }

    // Object maybe doens't neet to be tranasferred after creation
    if (!object->_transferrable) {
        return object;
    }

    // If we just did a transfer, return the object after doing post-transfer work
    if (GLTexture::Transferred == object->getSyncState()) {
        object->postTransfer();
        return object;
    }

    if (object->isReady()) {
        // Do we need to reduce texture memory usage?
        if (object->isOverMaxMemory() && texturePointer->incremementMinMip()) {
            // This automatically destroys the old texture
            object = new GLTexture(*object, texture);
            _textureTransferHelper->transferTexture(texturePointer);
        }
    } else if (object->isOutdated()) {
        // Object might be outdated, if so, start the transfer
        // (outdated objects that are already in transfer will have reported 'true' for ready()
        _textureTransferHelper->transferTexture(texturePointer);
    }

    return object;
}

std::shared_ptr<GLTextureTransferHelper> GLBackend::_textureTransferHelper;

void GLBackend::initTextureTransferHelper() {
    _textureTransferHelper = std::make_shared<GLTextureTransferHelper>();
}

GLuint GLBackend::getTextureID(const TexturePointer& texture, bool sync) {
    if (!texture) {
        return 0;
    }
    GLTexture* object { nullptr };
    if (sync) {
        object = GLBackend::syncGPUObject(texture);
    } else {
        object = Backend::getGPUObject<GLBackend::GLTexture>(*texture);
    }
    if (object) {
        if (object->getSyncState() == GLTexture::Idle) {
            return object->_texture;
        } else if (object->_downsampleSource) {
            return object->_downsampleSource->_texture;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

void GLBackend::syncSampler(const Sampler& sampler, Texture::Type type, const GLTexture* object) {
    if (!object) return;

    class GLFilterMode {
    public:
        GLint minFilter;
        GLint magFilter;
    };
    static const GLFilterMode filterModes[Sampler::NUM_FILTERS] = {
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

    auto fm = filterModes[sampler.getFilter()];
    glTexParameteri(object->_target, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTexParameteri(object->_target, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    static const GLenum comparisonFuncs[NUM_COMPARISON_FUNCS] = {
        GL_NEVER,
        GL_LESS,
        GL_EQUAL,
        GL_LEQUAL,
        GL_GREATER,
        GL_NOTEQUAL,
        GL_GEQUAL,
        GL_ALWAYS };

    if (sampler.doComparison()) {
        glTexParameteri(object->_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(object->_target, GL_TEXTURE_COMPARE_FUNC, comparisonFuncs[sampler.getComparisonFunction()]);
    } else {
        glTexParameteri(object->_target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    static const GLenum wrapModes[Sampler::NUM_WRAP_MODES] = {
        GL_REPEAT,                         // WRAP_REPEAT,
        GL_MIRRORED_REPEAT,                // WRAP_MIRROR,
        GL_CLAMP_TO_EDGE,                  // WRAP_CLAMP,
        GL_CLAMP_TO_BORDER,                // WRAP_BORDER,
        GL_MIRROR_CLAMP_TO_EDGE_EXT };     // WRAP_MIRROR_ONCE,

    glTexParameteri(object->_target, GL_TEXTURE_WRAP_S, wrapModes[sampler.getWrapModeU()]);
    glTexParameteri(object->_target, GL_TEXTURE_WRAP_T, wrapModes[sampler.getWrapModeV()]);
    glTexParameteri(object->_target, GL_TEXTURE_WRAP_R, wrapModes[sampler.getWrapModeW()]);

    glTexParameterfv(object->_target, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    glTexParameteri(object->_target, GL_TEXTURE_BASE_LEVEL, (uint16)sampler.getMipOffset());
    glTexParameterf(object->_target, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTexParameterf(object->_target, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
    glTexParameterf(object->_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
}

void GLBackend::do_generateTextureMips(Batch& batch, size_t paramOffset) {
    TexturePointer resourceTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);
    if (!resourceTexture) {
        return;
    }

    // DO not transfer the texture, this call is expected for rendering texture
    GLTexture* object = GLBackend::syncGPUObject(resourceTexture, false);
    if (!object) {
        return;
    }

    // IN 4.1 we still need to find an available slot
    auto freeSlot = _resource.findEmptyTextureSlot();
    auto bindingSlot = (freeSlot < 0 ? 0 : freeSlot);
    glActiveTexture(GL_TEXTURE0 + bindingSlot);
    glBindTexture(object->_target, object->_texture);

    glGenerateMipmap(object->_target);

    if (freeSlot < 0) {
        // If had to use slot 0 then restore state
        GLTexture* boundObject = GLBackend::syncGPUObject(_resource._textures[0]);
        if (boundObject) {
            glBindTexture(boundObject->_target, boundObject->_texture);
        }
    } else {
        // clean up
        glBindTexture(object->_target, 0);
    }
    (void)CHECK_GL_ERROR();
}
