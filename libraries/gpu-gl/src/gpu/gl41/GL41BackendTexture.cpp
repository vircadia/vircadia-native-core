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
                object = new GL41ResourceTexture(shared_from_this(), texture);
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
    glActiveTexture(GL_TEXTURE0 + GL41Backend::RESOURCE_TRANSFER_TEX_UNIT);
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
        glTexSubImage2D(_target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
    } else if (GL_TEXTURE_CUBE_MAP == _target) {
        auto target = GLTexture::CUBE_FACE_LAYOUT[face];
        glTexSubImage2D(target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
    } else {
        assert(false);
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
}

using GL41FixedAllocationTexture = GL41Backend::GL41FixedAllocationTexture;

GL41FixedAllocationTexture::GL41FixedAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41Texture(backend, texture), _size(texture.evalTotalSize()) {
    withPreservedTexture([&] {
        allocateStorage();
        syncSampler();
    });
}

GL41FixedAllocationTexture::~GL41FixedAllocationTexture() {
}

void GL41FixedAllocationTexture::allocateStorage() const {
    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const auto numMips = _gpuObject.getNumMips();

    // glTextureStorage2D(_id, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);
    for (GLint level = 0; level < numMips; level++) {
        Vec3u dimensions = _gpuObject.evalMipDimensions(level);
        for (GLenum target : getFaceTargets(_target)) {
            glTexImage2D(target, level, texelFormat.internalFormat, dimensions.x, dimensions.y, 0, texelFormat.format, texelFormat.type, nullptr);
        }
    }

    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(_target, GL_TEXTURE_MAX_LEVEL, numMips - 1);
}

void GL41FixedAllocationTexture::syncSampler() const {
    Parent::syncSampler();
    const Sampler& sampler = _gpuObject.getSampler();
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), sampler.getMinMip());

    glTexParameteri(_target, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTexParameterf(_target, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTexParameterf(_target, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.0f : sampler.getMaxMip()));
}

// Renderbuffer attachment textures
using GL41AttachmentTexture = GL41Backend::GL41AttachmentTexture;

GL41AttachmentTexture::GL41AttachmentTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41FixedAllocationTexture(backend, texture) {
    Backend::updateTextureGPUFramebufferMemoryUsage(0, size());
}

GL41AttachmentTexture::~GL41AttachmentTexture() {
    Backend::updateTextureGPUFramebufferMemoryUsage(size(), 0);
}

// Strict resource textures
using GL41StrictResourceTexture = GL41Backend::GL41StrictResourceTexture;

GL41StrictResourceTexture::GL41StrictResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41FixedAllocationTexture(backend, texture) {
    withPreservedTexture([&] {
   
        auto mipLevels = _gpuObject.getNumMips();
        for (uint16_t sourceMip = 0; sourceMip < mipLevels; sourceMip++) {
            uint16_t targetMip = sourceMip;
            size_t maxFace = GLTexture::getFaceCount(_target);
            for (uint8_t face = 0; face < maxFace; face++) {
                copyMipFaceFromTexture(sourceMip, targetMip, face);
            }
        }
    });

    if (texture.isAutogenerateMips()) {
        generateMips();
    }
}

// resource textures
using GL41ResourceTexture = GL41Backend::GL41ResourceTexture;

GL41ResourceTexture::GL41ResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL41FixedAllocationTexture(backend, texture) {
    Backend::updateTextureGPUMemoryUsage(0, size());

    withPreservedTexture([&] {
   
        auto mipLevels = _gpuObject.getNumMips();
        for (uint16_t sourceMip = 0; sourceMip < mipLevels; sourceMip++) {
            uint16_t targetMip = sourceMip;
            size_t maxFace = GLTexture::getFaceCount(_target);
            for (uint8_t face = 0; face < maxFace; face++) {
                copyMipFaceFromTexture(sourceMip, targetMip, face);
            }
        }
    });

    if (texture.isAutogenerateMips()) {
        generateMips();
    }
}

GL41ResourceTexture::~GL41ResourceTexture() {
    Backend::updateTextureGPUMemoryUsage(size(), 0);
}
