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

#include <unordered_set>
#include <unordered_map>
#include <QtCore/QThread>

#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl45;

using GLTexelFormat = gl::GLTexelFormat;
using GL45Texture = GL45Backend::GL45Texture;

GLuint GL45Texture::allocate(const Texture& texture) {
    Backend::incrementTextureGPUCount();
    GLuint result;
    glCreateTextures(getGLTextureType(texture), 1, &result);
    return result;
}

GLuint GL45Backend::getTextureID(const TexturePointer& texture, bool transfer) const {
    return GL45Texture::getId<GL45Texture>(texture, transfer);
}

gl::GLTexture* GL45Backend::syncGPUObject(const TexturePointer& texture, bool transfer) const {
    return GL45Texture::sync<GL45Texture>(texture, transfer);
}

GL45Backend::GL45Texture::GL45Texture(const Texture& texture, bool transferrable) 
    : gl::GLTexture(texture, allocate(texture), transferrable) {}

GL45Backend::GL45Texture::GL45Texture(const Texture& texture, GLTexture* original) 
    : gl::GLTexture(texture, allocate(texture), original) {}

void GL45Backend::GL45Texture::withPreservedTexture(std::function<void()> f) const {
    f();
}

void GL45Backend::GL45Texture::generateMips() const {
    glGenerateTextureMipmap(_id);
    (void)CHECK_GL_ERROR();
}

void GL45Backend::GL45Texture::allocateStorage() const {
    gl::GLTexelFormat texelFormat = gl::GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }
    // Get the dimensions, accounting for the downgrade level
    Vec3u dimensions = _gpuObject.evalMipDimensions(_minMip);
    glTextureStorage2D(_id, usedMipLevels(), texelFormat.internalFormat, dimensions.x, dimensions.y);
    (void)CHECK_GL_ERROR();
}

void GL45Backend::GL45Texture::updateSize() const {
    setSize(_virtualSize);
    if (!_id) {
        return;
    }

    if (_gpuObject.getTexelFormat().isCompressed()) {
        qFatal("Compressed textures not yet supported");
    }
}

// Move content bits from the CPU to the GPU for a given mip / face
void GL45Backend::GL45Texture::transferMip(uint16_t mipLevel, uint8_t face) const {
    auto mip = _gpuObject.accessStoredMipFace(mipLevel, face);
    gl::GLTexelFormat texelFormat = gl::GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), mip->getFormat());
    auto size = _gpuObject.evalMipDimensions(mipLevel);
    if (GL_TEXTURE_2D == _target) {
        glTextureSubImage2D(_id, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
    } else if (GL_TEXTURE_CUBE_MAP == _target) {
        glTextureSubImage3D(_id, mipLevel, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mip->readData());
    } else {
        Q_ASSERT(false);
    }
    (void)CHECK_GL_ERROR();
}

// This should never happen on the main thread
// Move content bits from the CPU to the GPU
void GL45Backend::GL45Texture::transfer() const {
    PROFILE_RANGE(__FUNCTION__);
    //qDebug() << "Transferring texture: " << _privateTexture;
    // Need to update the content of the GPU object from the source sysmem of the texture
    if (_contentStamp >= _gpuObject.getDataStamp()) {
        return;
    }

    if (_downsampleSource._texture) {
        GLuint fbo { 0 };
        glCreateFramebuffers(1, &fbo);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        // Find the distance between the old min mip and the new one
        uint16 mipOffset = _minMip - _downsampleSource._minMip;
        for (uint16 i = _minMip; i <= _maxMip; ++i) {
            uint16 targetMip = i - _minMip;
            uint16 sourceMip = targetMip + mipOffset;
            Vec3u dimensions = _gpuObject.evalMipDimensions(i);
            for (GLenum target : getFaceTargets(_target)) {
                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _downsampleSource._texture, sourceMip);
                (void)CHECK_GL_ERROR();
                glCopyTextureSubImage2D(_id, targetMip, 0, 0, 0, 0, dimensions.x, dimensions.y);
                (void)CHECK_GL_ERROR();
            }
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
    } else {
        // GO through the process of allocating the correct storage and/or update the content
        switch (_gpuObject.getType()) {
        case Texture::TEX_2D: 
            {
                for (uint16_t i = _minMip; i <= _maxMip; ++i) {
                    if (_gpuObject.isStoredMipFaceAvailable(i)) {
                        transferMip(i);
                    }
                }
            }
            break;

        case Texture::TEX_CUBE:
            // transfer pixels from each faces
            for (uint8_t f = 0; f < CUBE_NUM_FACES; f++) {
                for (uint16_t i = 0; i < Sampler::MAX_MIP_LEVEL; ++i) {
                    if (_gpuObject.isStoredMipFaceAvailable(i, f)) {
                        transferMip(i, f);
                    }
                }
            }
            break;

        default:
            qCWarning(gpugl45logging) << __FUNCTION__ << " case for Texture Type " << _gpuObject.getType() << " not supported";
            break;
        }
    }
    if (_gpuObject.isAutogenerateMips()) {
        glGenerateTextureMipmap(_id);
        (void)CHECK_GL_ERROR();
    }
}

void GL45Backend::GL45Texture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();

    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTextureParameteri(_id, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTextureParameteri(_id, GL_TEXTURE_MAG_FILTER, fm.magFilter);
    
    if (sampler.doComparison()) {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_FUNC, gl::COMPARISON_TO_GL[sampler.getComparisonFunction()]);
    } else {
        glTextureParameteri(_id, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    glTextureParameteri(_id, GL_TEXTURE_WRAP_S, WRAP_MODES[sampler.getWrapModeU()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_T, WRAP_MODES[sampler.getWrapModeV()]);
    glTextureParameteri(_id, GL_TEXTURE_WRAP_R, WRAP_MODES[sampler.getWrapModeW()]);
    glTextureParameterfv(_id, GL_TEXTURE_BORDER_COLOR, (const float*)&sampler.getBorderColor());
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, (uint16)sampler.getMipOffset());
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
    glTextureParameterf(_id, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
}

