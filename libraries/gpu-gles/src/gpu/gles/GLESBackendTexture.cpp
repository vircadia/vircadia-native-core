//
//  GLESBackendTexture.cpp
//  libraries/gpu-gl-android/src/gpu/gles
//
//  Created by Gabriel Calero & Cristian Duarte on 9/27/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLESBackend.h"

#include <unordered_set>
#include <unordered_map>
#include <QtCore/QThread>

// #include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gles;

//using GL41TexelFormat = GLTexelFormat;
using GLESTexture = GLESBackend::GLESTexture;

GLuint GLESTexture::allocate() {
    //CLIMAX_MERGE_START
    //Backend::incrementTextureGPUCount();
    //CLIMAX_MERGE_END
    GLuint result;
    glGenTextures(1, &result);
    return result;
}

GLuint GLESBackend::getTextureID(const TexturePointer& texture, bool transfer) {
    return GLESTexture::getId<GLESTexture>(*this, texture, transfer);
}

GLTexture* GLESBackend::syncGPUObject(const TexturePointer& texture, bool transfer) {
    return GLESTexture::sync<GLESTexture>(*this, texture, transfer);
}

GLESTexture::GLESTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint externalId) 
    : GLTexture(backend, texture, externalId) {
}

GLESTexture::GLESTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, bool transferrable) 
    : GLTexture(backend, texture, allocate(), transferrable) {
}

void GLESTexture::generateMips() const {
    withPreservedTexture([&] {
        glGenerateMipmap(_target);
    });
    (void)CHECK_GL_ERROR();
}

void GLESTexture::allocateStorage() const {
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, 0);
    (void)CHECK_GL_ERROR();
    glTexParameteri(_target, GL_TEXTURE_MAX_LEVEL, _maxMip - _minMip);
    (void)CHECK_GL_ERROR();
/*    if (GLEW_VERSION_4_2 && !_gpuObject.getTexelFormat().isCompressed()) {
        // Get the dimensions, accounting for the downgrade level
        Vec3u dimensions = _gpuObject.evalMipDimensions(_minMip);
        glTexStorage2D(_target, usedMipLevels(), texelFormat.internalFormat, dimensions.x, dimensions.y);
        (void)CHECK_GL_ERROR();
    } else {*/
        for (uint16_t l = _minMip; l <= _maxMip; l++) {
            // Get the mip level dimensions, accounting for the downgrade level
            Vec3u dimensions = _gpuObject.evalMipDimensions(l);
            for (GLenum target : getFaceTargets(_target)) {
                glTexImage2D(target, l - _minMip, texelFormat.internalFormat, dimensions.x, dimensions.y, 0, texelFormat.format, texelFormat.type, NULL);
                (void)CHECK_GL_ERROR();
            }
        }
    //}
}

void GLESTexture::updateSize() const {
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
                //glGetTexLevelParameteriv(proxyType, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &levelSize);
                //qDebug() << "TODO: GLBackendTexture.cpp:updateSize GL_TEXTURE_COMPRESSED_IMAGE_SIZE";
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
void GLESTexture::transferMip(uint16_t mipLevel, uint8_t face) const {
    auto mip = _gpuObject.accessStoredMipFace(mipLevel, face);
    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), _gpuObject.getStoredMipFormat());
    //GLenum target = getFaceTargets()[face];
    GLenum target = _target == GL_TEXTURE_2D ? GL_TEXTURE_2D : CUBE_FACE_LAYOUT[face];
    auto size = _gpuObject.evalMipDimensions(mipLevel);
    glTexSubImage2D(target, mipLevel, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mip->readData());
    (void)CHECK_GL_ERROR();
}

void GLESTexture::startTransfer() {
    PROFILE_RANGE(render_gpu_gl, __FUNCTION__);
    Parent::startTransfer();

    glBindTexture(_target, _id);
    (void)CHECK_GL_ERROR();

    // transfer pixels from each faces
    uint8_t numFaces = (Texture::TEX_CUBE == _gpuObject.getType()) ? CUBE_NUM_FACES : 1;
    for (uint8_t f = 0; f < numFaces; f++) {
        for (uint16_t i = 0; i < Sampler::MAX_MIP_LEVEL; ++i) {
            if (_gpuObject.isStoredMipFaceAvailable(i, f)) {
                transferMip(i, f);
            }
        }
    }
}

void GLESBackend::GLESTexture::syncSampler() const {
    const Sampler& sampler = _gpuObject.getSampler();
    const auto& fm = FILTER_MODES[sampler.getFilter()];
    glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    if (sampler.doComparison()) {
        glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(_target, GL_TEXTURE_COMPARE_FUNC, COMPARISON_TO_GL[sampler.getComparisonFunction()]);
    } else {
        glTexParameteri(_target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    glTexParameteri(_target, GL_TEXTURE_WRAP_S, WRAP_MODES[sampler.getWrapModeU()]);
    glTexParameteri(_target, GL_TEXTURE_WRAP_T, WRAP_MODES[sampler.getWrapModeV()]);
    glTexParameteri(_target, GL_TEXTURE_WRAP_R, WRAP_MODES[sampler.getWrapModeW()]);

    glTexParameterfv(_target, GL_TEXTURE_BORDER_COLOR_EXT, (const float*)&sampler.getBorderColor());
    

    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, (uint16)sampler.getMipOffset());
    glTexParameterf(_target, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTexParameterf(_target, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
    (void)CHECK_GL_ERROR();
    //qDebug() << "[GPU-GL-GLBackend] syncSampler 12 " << _target << "," << sampler.getMaxAnisotropy();
    //glTexParameterf(_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());
    //(void)CHECK_GL_ERROR();
    //qDebug() << "[GPU-GL-GLBackend] syncSampler end";

}

