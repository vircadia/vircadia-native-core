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

using GL41TexelFormat = GLTexelFormat;
using GL41Texture = GL41Backend::GL41Texture;

GLuint GL41Texture::allocate() {
    Backend::incrementTextureGPUCount();
    GLuint result;
    glGenTextures(1, &result);
    return result;
}

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
    }

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
    }

    return object;
}

GL41Texture::GL41Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) 
    : GLTexture(backend, texture, allocate()), _storageStamp { texture.getStamp() }, _size(texture.evalTotalSize()) {
    incrementTextureGPUCount();
    withPreservedTexture([&] {
        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat(), _gpuObject.getStoredMipFormat());
        auto numMips = _gpuObject.getNumMipLevels();
        for (uint16_t mipLevel = 0; mipLevel < numMips; ++mipLevel) {
            // Get the mip level dimensions, accounting for the downgrade level
            Vec3u dimensions = _gpuObject.evalMipDimensions(mipLevel);
            uint8_t face = 0;
            for (GLenum target : getFaceTargets(_target)) {
                const Byte* mipData = nullptr;
                if (_gpuObject.isStoredMipFaceAvailable(mipLevel, face)) {
                    auto mip = _gpuObject.accessStoredMipFace(mipLevel, face);
                    mipData = mip->readData();
                }
                glTexImage2D(target, mipLevel, texelFormat.internalFormat, dimensions.x, dimensions.y, 0, texelFormat.format, texelFormat.type, mipData);
                (void)CHECK_GL_ERROR();
                ++face;
            }
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

void GL41Texture::withPreservedTexture(std::function<void()> f) const {
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

void GL41Texture::generateMips() const {
    withPreservedTexture([&] {
        glGenerateMipmap(_target);
    });
    (void)CHECK_GL_ERROR();
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
