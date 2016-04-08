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
#include "GPULogging.h"

#include <QtCore/QThread>

#include "GLBackendShared.h"
#include "GLBackendTextureTransfer.h"

using namespace gpu;

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
    GLuint result;
    glGenTextures(1, &result);
    return result;
}

const GLenum GLBackend::GLTexture::CUBE_FACE_LAYOUT[6] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

// Create the texture and allocate storage
GLBackend::GLTexture::GLTexture(const Texture& texture) : 
    _storageStamp(texture.getStamp()),
    _target(gpuToGLTextureType(texture)),
    _size(0),
    _virtualSize(0),
    _numLevels(texture.maxMip() + 1),
    _gpuTexture(texture) 
{
    Backend::incrementTextureGPUCount();
    Backend::setGPUObject(texture, this);


   // updateSize();
    GLuint virtualSize = _gpuTexture.evalTotalSize();
    setVirtualSize(virtualSize);
    setSize(virtualSize);
}

void GLBackend::GLTexture::createTexture() {
    _privateTexture = allocateSingleTexture();

    GLsizei width = _gpuTexture.getWidth();
    GLsizei height = _gpuTexture.getHeight();

    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuTexture.getTexelFormat());

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

    glBindTexture(_target, _privateTexture);

    (void)CHECK_GL_ERROR();
    // Fixme: this usage of TexStorage doesn;t work wtih compressed texture, altuogh it should.
    // GO through the process of allocating the correct storage 
    /*  if (GLEW_VERSION_4_2 && !texture.getTexelFormat().isCompressed()) {
    glTexStorage2D(_target, _numLevels, texelFormat.internalFormat, width, height);
    (void)CHECK_GL_ERROR();
    } else*/ 
    {
        glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(_target, GL_TEXTURE_MAX_LEVEL, _numLevels - 1);

        // for (int l = 0; l < _numLevels; l++) {
        { int l = 0;
        if (_gpuTexture.getType() == gpu::Texture::TEX_CUBE) {
            for (size_t face = 0; face < CUBE_NUM_FACES; face++) {
                glTexImage2D(CUBE_FACE_LAYOUT[face], l, texelFormat.internalFormat, width, height, 0, texelFormat.format, texelFormat.type, NULL);
            }
        } else {
            glTexImage2D(_target, l, texelFormat.internalFormat, width, height, 0, texelFormat.format, texelFormat.type, NULL);
        }
        width = std::max(1, (width / 2));
        height = std::max(1, (height / 2));
        }
        (void)CHECK_GL_ERROR();
    }

    syncSampler(_gpuTexture.getSampler(), _gpuTexture.getType(), this);
    (void)CHECK_GL_ERROR();


    glBindTexture(_target, boundTex);
    (void)CHECK_GL_ERROR();
}

GLBackend::GLTexture::~GLTexture() {
    if (_privateTexture != 0) {
        glDeleteTextures(1, &_privateTexture);
    }
 
    Backend::updateTextureGPUMemoryUsage(_size, 0);
    Backend::updateTextureGPUVirtualMemoryUsage(_virtualSize, 0);
    Backend::decrementTextureGPUCount();
}


void GLBackend::GLTexture::setSize(GLuint size) {
    Backend::updateTextureGPUMemoryUsage(_size, size);
    _size = size;
}

void GLBackend::GLTexture::setVirtualSize(GLuint size) {
    Backend::updateTextureGPUVirtualMemoryUsage(_virtualSize, size);
    _virtualSize = size;
}

void GLBackend::GLTexture::updateSize() {
    GLuint virtualSize = _gpuTexture.evalTotalSize();
    setVirtualSize(virtualSize);
    if (!_texture) {
        setSize(virtualSize);
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
            for (GLuint level = 0; level < _numLevels; level++) {
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
        } else {
            setSize(virtualSize);
        }

    } else {
        setSize(virtualSize);
    }
}


bool GLBackend::GLTexture::isInvalid() const {
    return _storageStamp < _gpuTexture.getStamp();
}

bool GLBackend::GLTexture::isOutdated() const {
    return _contentStamp < _gpuTexture.getDataStamp();
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
        return Pending == syncState;
    }

    return Idle == syncState;
}

// Move content bits from the CPU to the GPU for a given mip / face
void GLBackend::GLTexture::transferMip(GLenum target, const Texture::PixelsPointer& mip) const {
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuTexture.getTexelFormat(), mip->getFormat());
    glTexSubImage2D(target, 0, 0, 0, _gpuTexture.getWidth(), _gpuTexture.getHeight(), texelFormat.format, texelFormat.type, mip->readData());		      glTexSubImage2D(target, 0, 0, 0, _gpuTexture.getWidth(), _gpuTexture.getHeight(), texelFormat.format, texelFormat.type, mip->readData());
    (void)CHECK_GL_ERROR();
}

// Move content bits from the CPU to the GPU
void GLBackend::GLTexture::transfer() const {
    PROFILE_RANGE(__FUNCTION__);
    //qDebug() << "Transferring texture: " << _privateTexture;
    // Need to update the content of the GPU object from the source sysmem of the texture
    if (_contentStamp >= _gpuTexture.getDataStamp()) {
        return;
    }

    //_secretTexture
    glBindTexture(_target, _privateTexture);
    // glBindTexture(_target, _texture);
    // GO through the process of allocating the correct storage and/or update the content
    switch (_gpuTexture.getType()) {
        case Texture::TEX_2D:
            if (_gpuTexture.isStoredMipFaceAvailable(0)) {
                transferMip(GL_TEXTURE_2D, _gpuTexture.accessStoredMipFace(0));
            }
            break;

        case Texture::TEX_CUBE:
            // transfer pixels from each faces
            for (uint8_t f = 0; f < CUBE_NUM_FACES; f++) {
                if (_gpuTexture.isStoredMipFaceAvailable(0, f)) {
                    transferMip(CUBE_FACE_LAYOUT[f], _gpuTexture.accessStoredMipFace(0, f));
                }
            }
            break;

        default:
            qCWarning(gpulogging) << __FUNCTION__ << " case for Texture Type " << _gpuTexture.getType() << " not supported";
            break;
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

    // At this point the mip pixels have been loaded, we can notify the gpu texture to abandon it's memory
    switch (_gpuTexture.getType()) {
        case Texture::TEX_2D:
            _gpuTexture.notifyMipFaceGPULoaded(0, 0);
            break;

        case Texture::TEX_CUBE:
            for (uint8_t f = 0; f < CUBE_NUM_FACES; ++f) {
                _gpuTexture.notifyMipFaceGPULoaded(0, f);
            }
            break;

        default:
            qCWarning(gpulogging) << __FUNCTION__ << " case for Texture Type " << _gpuTexture.getType() << " not supported";
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
    if (object && object->isReady()) {
        return object;
    }

    // Object isn't ready, check what we need to do...

    // Create the texture if need be (force re-creation if the storage stamp changes
    // for easier use of immutable storage)
    if (!object || object->isInvalid()) {
        // This automatically destroys the old texture
        object = new GLTexture(texture);
    }

    // Object maybe doens't neet to be tranasferred after creation
    if (!needTransfer) {
        object->createTexture();
        object->_contentStamp = texturePointer->getDataStamp();
        object->postTransfer();
        return object;
    }

    // Object might be outdated, if so, start the transfer
    // (outdated objects that are already in transfer will have reported 'true' for ready()
    if (object->isOutdated()) {
        Backend::incrementTextureGPUTransferCount();
        _textureTransferHelper->transferTexture(texturePointer);
    }

    if (GLTexture::Transferred == object->getSyncState()) {
        Backend::decrementTextureGPUTransferCount();
        object->postTransfer();
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
        return object->_texture;
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
    static const GLFilterMode filterModes[] = {
        { GL_NEAREST, GL_NEAREST },  //FILTER_MIN_MAG_POINT,
        { GL_NEAREST, GL_LINEAR },  //FILTER_MIN_POINT_MAG_LINEAR,
        { GL_LINEAR, GL_NEAREST },  //FILTER_MIN_LINEAR_MAG_POINT,
        { GL_LINEAR, GL_LINEAR },  //FILTER_MIN_MAG_LINEAR,

        { GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },  //FILTER_MIN_MAG_MIP_POINT,
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

    static const GLenum comparisonFuncs[] = {
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

    static const GLenum wrapModes[] = {
        GL_REPEAT,                         // WRAP_REPEAT,
        GL_MIRRORED_REPEAT,                // WRAP_MIRROR,
        GL_CLAMP_TO_EDGE,                  // WRAP_CLAMP,
        GL_CLAMP_TO_BORDER,                // WRAP_BORDER,
        GL_MIRROR_CLAMP_TO_EDGE_EXT };     // WRAP_MIRROR_ONCE,

    glTexParameteri(object->_target, GL_TEXTURE_WRAP_S, wrapModes[sampler.getWrapModeU()]);
    glTexParameteri(object->_target, GL_TEXTURE_WRAP_T, wrapModes[sampler.getWrapModeV()]);
    glTexParameteri(object->_target, GL_TEXTURE_WRAP_R, wrapModes[sampler.getWrapModeW()]);

    glTexParameterfv(object->_target, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    glTexParameteri(object->_target, GL_TEXTURE_BASE_LEVEL, sampler.getMipOffset());
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
