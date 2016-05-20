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
#include <QtCore/QThread>

#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl41;

using GL41TexelFormat = gl::GLTexelFormat;
using GL41Texture = GL41Backend::GL41Texture;

GLuint GL41Texture::allocate() {
    Backend::incrementTextureGPUCount();
    GLuint result;
    glGenTextures(1, &result);
    return result;
}

GLuint GL41Backend::getTextureID(const TexturePointer& texture, bool transfer) {
    return GL41Texture::getId<GL41Texture>(texture, transfer);
}

gl::GLTexture* GL41Backend::syncGPUObject(const TexturePointer& texture, bool transfer) {
    return GL41Texture::sync<GL41Texture>(texture, transfer);
}

GL41Texture::GL41Texture(const Texture& texture, bool transferrable) : gl::GLTexture(texture, allocate(), transferrable) {}

GL41Texture::GL41Texture(const Texture& texture, GL41Texture* original) : gl::GLTexture(texture, allocate(), original) {}

void GL41Backend::GL41Texture::withPreservedTexture(std::function<void()> f) const  {
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

    glBindTexture(_target, _texture);
    f();
    glBindTexture(_target, boundTex);
    (void)CHECK_GL_ERROR();
}

void GL41Backend::GL41Texture::generateMips() const {
    withPreservedTexture([&] {
        glGenerateMipmap(_target);
    });
    (void)CHECK_GL_ERROR();
}

void GL41Backend::GL41Texture::allocateStorage() const {
    gl::GLTexelFormat texelFormat = gl::GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, 0);
    (void)CHECK_GL_ERROR();
    glTexParameteri(_target, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    (void)CHECK_GL_ERROR();
    if (GLEW_VERSION_4_2 && !_gpuObject.getTexelFormat().isCompressed()) {
        // Get the dimensions, accounting for the downgrade level
        Vec3u dimensions = _gpuObject.evalMipDimensions(_minMip);
        glTexStorage2D(_target, usedMipLevels(), texelFormat.internalFormat, dimensions.x, dimensions.y);
        (void)CHECK_GL_ERROR();
    } else {
        for (uint16_t l = _minMip; l <= _maxMip; l++) {
            // Get the mip level dimensions, accounting for the downgrade level
            Vec3u dimensions = _gpuObject.evalMipDimensions(l);
            for (GLenum target : getFaceTargets(_target)) {
                glTexImage2D(target, l - _minMip, texelFormat.internalFormat, dimensions.x, dimensions.y, 0, texelFormat.format, texelFormat.type, NULL);
                (void)CHECK_GL_ERROR();
            }
        }
    }
}

void GL41Backend::GL41Texture::updateSize() const {
    setSize(_virtualSize);
    if (!_id) {
        return;
    }

    if (_gpuObject.getTexelFormat().isCompressed()) {
        GLenum proxyType = GL_TEXTURE_2D;
        GLuint numFaces = 1;
        if (_gpuObject.getType() == gpu::Texture::TEX_CUBE) {
            proxyType = CUBE_FACE_LAYOUT[0];
            numFaces = (GLuint)CUBE_NUM_FACES;
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

// Move content bits from the CPU to the GPU for a given mip / face
void GL41Backend::GL41Texture::transferMip(uint16_t mipLevel, uint8_t face) const {
    auto mip = _gpuObject.accessStoredMipFace(mipLevel, face);
    gl::GLTexelFormat texelFormat = gl::GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), mip->getFormat());
    //GLenum target = getFaceTargets()[face];
    GLenum target = _target == GL_TEXTURE_2D ? GL_TEXTURE_2D : CUBE_FACE_LAYOUT[face];
    auto size = _gpuObject.evalMipDimensions(mipLevel);
    glTexSubImage2D(target, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
    (void)CHECK_GL_ERROR();
}

// This should never happen on the main thread
// Move content bits from the CPU to the GPU
void GL41Backend::GL41Texture::transfer() const {
    PROFILE_RANGE(__FUNCTION__);
    //qDebug() << "Transferring texture: " << _privateTexture;
    // Need to update the content of the GPU object from the source sysmem of the texture
    if (_contentStamp >= _gpuObject.getDataStamp()) {
        return;
    }

    glBindTexture(_target, _id);
    (void)CHECK_GL_ERROR();

    if (_downsampleSource._texture) {
        GLuint fbo { 0 };
        glGenFramebuffers(1, &fbo);
        (void)CHECK_GL_ERROR();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        (void)CHECK_GL_ERROR();
        // Find the distance between the old min mip and the new one
        uint16 mipOffset = _minMip - _downsampleSource._minMip;
        for (uint16 i = _minMip; i <= _maxMip; ++i) {
            uint16 targetMip = i - _minMip;
            uint16 sourceMip = targetMip + mipOffset;
            Vec3u dimensions = _gpuObject.evalMipDimensions(i);
            for (GLenum target : getFaceTargets(_target)) {
                glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, _downsampleSource._texture, sourceMip);
                (void)CHECK_GL_ERROR();
                glCopyTexSubImage2D(target, targetMip, 0, 0, 0, 0, dimensions.x, dimensions.y);
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
            qCWarning(gpugl41logging) << __FUNCTION__ << " case for Texture Type " << _gpuObject.getType() << " not supported";
            break;
        }
    }
    if (_gpuObject.isAutogenerateMips()) {
        glGenerateMipmap(_target);
        (void)CHECK_GL_ERROR();
    }
}

void GL41Backend::GL41Texture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();
    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    if (sampler.doComparison()) {
        glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(_target, GL_TEXTURE_COMPARE_FUNC, gl::COMPARISON_TO_GL[sampler.getComparisonFunction()]);
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

