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

        case TextureUsageType::RESOURCE: {
            qCDebug(gpugllogging) << "variable / Strict texture " << texture.source().c_str();
            object = new GL41StrictResourceTexture(shared_from_this(), texture);
            break;
        }

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
    glGenTextures(1, &result);
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
        glTextureSubImage2D(_id, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
    } else if (GL_TEXTURE_CUBE_MAP == _target) {
        // DSA ARB does not work on AMD, so use EXT
        // unless EXT is not available on the driver
        if (glTextureSubImage2DEXT) {
            auto target = GLTexture::CUBE_FACE_LAYOUT[face];
            glTextureSubImage2DEXT(_id, target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
        } else {
            glTextureSubImage3D(_id, mip, 0, yOffset, face, size.x, size.y, 1, format, type, sourcePointer);
        }
    } else {
        Q_ASSERT(false);
    }
    (void)CHECK_GL_ERROR();
}

void GL41Texture::copyMipFaceFromTexture(uint16_t sourceMip, uint16_t targetMip, uint8_t face) const {
    if (!_gpuObject.isStoredMipFaceAvailable(sourceMip)) {
        return;
    }
    auto size = _gpuObject.evalMipDimensions(sourceMip);
    auto mipData = _gpuObject.accessStoredMipFace(sourceMip, face);
    if (mipData) {
        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), _gpuObject.getStoredMipFormat());
        copyMipFaceLinesFromTexture(targetMip, face, size, 0, texelFormat.format, texelFormat.type, mipData->readData());
    } else {
        qCDebug(gpugllogging) << "Missing mipData level=" << sourceMip << " face=" << (int)face << " for texture " << _gpuObject.source().c_str();
    }
}

void GL41Texture::syncSampler() const {
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
    glTextureParameterf(_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
    glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
}

using GL45FixedAllocationTexture = GL45Backend::GL45FixedAllocationTexture;

GL41FixedAllocationTexture::GL41FixedAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45Texture(backend, texture), _size(texture.evalTotalSize()) {
    allocateStorage();
    syncSampler();
}

GL41FixedAllocationTexture::~GL41FixedAllocationTexture() {
}

void GL41FixedAllocationTexture::allocateStorage() const {
    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const auto dimensions = _gpuObject.getDimensions();
    const auto mips = _gpuObject.getNumMips();

    glTextureStorage2D(_id, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
}

void GL41FixedAllocationTexture::syncSampler() const {
    Parent::syncSampler();
    const Sampler& sampler = _gpuObject.getSampler();
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), sampler.getMinMip());
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
}

// Renderbuffer attachment textures
using GL41AttachmentTexture = GL45Backend::GL41AttachmentTexture;

GL41AttachmentTexture::GL41AttachmentTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45FixedAllocationTexture(backend, texture) {
    Backend::updateTextureGPUFramebufferMemoryUsage(0, size());
}

GL41AttachmentTexture::~GL41AttachmentTexture() {
    Backend::updateTextureGPUFramebufferMemoryUsage(size(), 0);
}

// Strict resource textures
using GL41StrictResourceTexture = GL41Backend::GL41StrictResourceTexture;

GL41StrictResourceTexture::GL41StrictResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45FixedAllocationTexture(backend, texture) {
    auto mipLevels = _gpuObject.getNumMips();
    for (uint16_t sourceMip = 0; sourceMip < mipLevels; ++sourceMip) {
        uint16_t targetMip = sourceMip;
        size_t maxFace = GLTexture::getFaceCount(_target);
        for (uint8_t face = 0; face < maxFace; ++face) {
            copyMipFaceFromTexture(sourceMip, targetMip, face);
        }
    }
    if (texture.isAutogenerateMips()) {
        generateMips();
    }
}


using GL41TexelFormat = GLTexelFormat;
using GL41Texture = GL41Backend::GL41Texture;



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

    // If the object hasn't been created, or the object definition is out of date, drop and re-create
    GL41Texture* object = Backend::getGPUObject<GL41Texture>(texture);

    if (!object || object->_storageStamp < texture.getStamp()) {
        // This automatically any previous texture
        object = new GL41Texture(shared_from_this(), texture);
        object->withPreservedTexture([&] {
            if (object->_contentStamp <= texture.getDataStamp()) {
                // FIXME implement synchronous texture transfer here
                object->syncContent();
            }

            if (object->_samplerStamp <= texture.getSamplerStamp()) {
                object->syncSampler();
            }
        });
    }
    /*
    // FIXME internalize to GL41Texture 'sync' function
    if (object->isOutdated()) {
        object->withPreservedTexture([&] {
            if (object->_contentStamp <= texture.getDataStamp()) {
                // FIXME implement synchronous texture transfer here
                object->syncContent();
            }

            if (object->_samplerStamp <= texture.getSamplerStamp()) {
                object->syncSampler();
            }
        });
    }*/

    return object;
}

GL41Texture::GL41Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) 
    : GLTexture(backend, texture, allocate()), _storageStamp { texture.getStamp() }, _size(texture.evalTotalSize()) {
    incrementTextureGPUCount();
    withPreservedTexture([&] {
        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), _gpuObject.getStoredMipFormat());
        auto numMips = _gpuObject.getNumMips();
        for (uint16_t mipLevel = 0; mipLevel < numMips; ++mipLevel) {
            // Get the mip level dimensions, accounting for the downgrade level
            Vec3u dimensions = _gpuObject.evalMipDimensions(mipLevel);
            uint8_t face = 0;
            for (GLenum target : getFaceTargets(_target)) {
                const Byte* mipData = nullptr;
                gpu::Texture::PixelsPointer mip;
                if (_gpuObject.isStoredMipFaceAvailable(mipLevel, face)) {
                    mip = _gpuObject.accessStoredMipFace(mipLevel, face);
                    if (mip) {
                        mipData = mip->readData();
                    } else {
                        mipData = nullptr;
                    }
                }
                glTexImage2D(target, mipLevel, texelFormat.internalFormat, dimensions.x, dimensions.y, 0, texelFormat.format, texelFormat.type, mipData);
                (void)CHECK_GL_ERROR();
                ++face;
            }
        }
        glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(_target, GL_TEXTURE_MAX_LEVEL, numMips - 1);

        if (texture.isAutogenerateMips()) {
            glGenerateMipmap(_target);
            (void)CHECK_GL_ERROR();
        }
    });
}

GL41Texture::~GL41Texture() {

}

bool GL41Texture::isOutdated() const {
    if (_samplerStamp <= _gpuObject.getSamplerStamp()) {
        return true;
    }
    if (TextureUsageType::RESOURCE == _gpuObject.getUsageType() && _contentStamp <= _gpuObject.getDataStamp()) {
        return true;
    }
    return false;
}

void GL41Texture::syncContent() const {
    // FIXME actually copy the texture data
    _contentStamp = _gpuObject.getDataStamp() + 1;
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
    _samplerStamp = _gpuObject.getSamplerStamp() + 1;
}

uint32 GL41Texture::size() const {
    return _size;
}
