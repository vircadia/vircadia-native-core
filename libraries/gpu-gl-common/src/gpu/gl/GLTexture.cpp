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

static constexpr size_t MAX_PIXEL_BYTE_SIZE{ 4 };
static constexpr size_t MAX_TRANSFER_DIMENSION{ 1024 };

const uvec3 GLVariableAllocationSupport::MAX_TRANSFER_DIMENSIONS{ MAX_TRANSFER_DIMENSION, MAX_TRANSFER_DIMENSION, 1 };
const uvec3 GLVariableAllocationSupport::INITIAL_MIP_TRANSFER_DIMENSIONS{ 64, 64, 1 };
const size_t GLVariableAllocationSupport::MAX_TRANSFER_SIZE = MAX_TRANSFER_DIMENSION * MAX_TRANSFER_DIMENSION * MAX_PIXEL_BYTE_SIZE;
const size_t GLVariableAllocationSupport::MAX_BUFFER_SIZE = MAX_TRANSFER_SIZE;

GLenum GLTexture::getGLTextureType(const Texture& texture) {
    switch (texture.getType()) {
    case Texture::TEX_2D:
        if (!texture.isArray()) {
            if (!texture.isMultisample()) {
                return GL_TEXTURE_2D;
            } else {
                return GL_TEXTURE_2D_MULTISAMPLE;
            }
        } else {
            if (!texture.isMultisample()) {
                return GL_TEXTURE_2D_ARRAY;
            } else {
                return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
            }
        }
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
        case GL_TEXTURE_2D_MULTISAMPLE:
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            return TEXTURE_2D_NUM_FACES;
        case GL_TEXTURE_CUBE_MAP:
            return TEXTURE_CUBE_NUM_FACES;
        default:
            Q_UNREACHABLE();
            break;
    }
}
const std::vector<GLenum>& GLTexture::getFaceTargets(GLenum target) {
    static const std::vector<GLenum> cubeFaceTargets {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    static const std::vector<GLenum> face2DTargets {
        GL_TEXTURE_2D
    };
    static const std::vector<GLenum> face2DMSTargets{
        GL_TEXTURE_2D_MULTISAMPLE
    }; 
    static const std::vector<GLenum> arrayFaceTargets{
        GL_TEXTURE_2D_ARRAY 
    };
    switch (target) {
    case GL_TEXTURE_2D:
        return face2DTargets;
    case GL_TEXTURE_2D_MULTISAMPLE:
        return face2DMSTargets;
    case GL_TEXTURE_2D_ARRAY:
        return arrayFaceTargets;
    case GL_TEXTURE_CUBE_MAP:
        return cubeFaceTargets;
    default:
        Q_UNREACHABLE();
        break;
    }
    Q_UNREACHABLE();
    return face2DTargets;
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

GLVariableAllocationSupport::GLVariableAllocationSupport() {
    Backend::textureResourceCount.increment();
}

GLVariableAllocationSupport::~GLVariableAllocationSupport() {
    Backend::textureResourceCount.decrement();
    Backend::textureResourceGPUMemSize.update(_size, 0);
    Backend::textureResourcePopulatedGPUMemSize.update(_populatedSize, 0);
}

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

void GLVariableAllocationSupport::sanityCheck() const {
    if (_populatedMip < _allocatedMip) {
        qCWarning(gpugllogging) << "Invalid mip levels";
    }
}

TransferJob::TransferJob(const Texture& texture,
    uint16_t sourceMip,
    uint16_t targetMip,
    uint8_t face,
    uint32_t lines,
    uint32_t lineOffset) {
    auto transferDimensions = texture.evalMipDimensions(sourceMip);
    GLenum format;
    GLenum internalFormat;
    GLenum type;
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), texture.getStoredMipFormat());
    format = texelFormat.format;
    internalFormat = texelFormat.internalFormat;
    type = texelFormat.type;
    _transferSize = texture.getStoredMipFaceSize(sourceMip, face);

    // If we're copying a subsection of the mip, do additional calculations to find the size and offset of the segment
    if (0 != lines) {
        transferDimensions.y = lines;
        auto dimensions = texture.evalMipDimensions(sourceMip);
        auto bytesPerLine = (uint32_t)_transferSize / dimensions.y;
        _transferOffset = bytesPerLine * lineOffset;
        _transferSize = bytesPerLine * lines;
    }

    Backend::texturePendingGPUTransferMemSize.update(0, _transferSize);

    if (_transferSize > GLVariableAllocationSupport::MAX_TRANSFER_SIZE) {
        qCWarning(gpugllogging) << "Transfer size of " << _transferSize << " exceeds theoretical maximum transfer size";
    }

    // Buffering can invoke disk IO, so it should be off of the main and render threads
    _bufferingLambda = [=](const TexturePointer& texture) {
        auto mipStorage = texture->accessStoredMipFace(sourceMip, face);
        if (mipStorage) {
            _mipData = mipStorage->createView(_transferSize, _transferOffset);
        } else {
            qCWarning(gpugllogging) << "Buffering failed because mip could not be retrieved from texture "
                << texture->source().c_str();
        }
    };

    _transferLambda = [=](const TexturePointer& texture) {
        if (_mipData) {
            auto gltexture = Backend::getGPUObject<GLTexture>(*texture);
            gltexture->copyMipFaceLinesFromTexture(targetMip, face, transferDimensions, lineOffset, internalFormat, format,
                type, _mipData->size(), _mipData->readData());
            _mipData.reset();
        } else {
            qCWarning(gpugllogging) << "Transfer failed because mip could not be retrieved from texture "
                << texture->source().c_str();
        }
    };
}

TransferJob::TransferJob(uint16_t sourceMip, const std::function<void()>& transferLambda) :
    _sourceMip(sourceMip), _bufferingRequired(false), _transferLambda([=](const TexturePointer&) { transferLambda(); }) {}

TransferJob::~TransferJob() {
    Backend::texturePendingGPUTransferMemSize.update(_transferSize, 0);
}

