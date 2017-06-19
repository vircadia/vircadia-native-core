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
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include <unordered_set>
#include <unordered_map>
#include <glm/gtx/component_wise.hpp>

#include <QtCore/QDebug>

#include <NumericalConstants.h>
#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

#define SPARSE_PAGE_SIZE_OVERHEAD_ESTIMATE 1.3f
#define MAX_RESOURCE_TEXTURES_PER_FRAME 2

GLTexture* GL45Backend::syncGPUObject(const TexturePointer& texturePointer) {
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

    GL45Texture* object = Backend::getGPUObject<GL45Texture>(texture);
    if (!object) {
        switch (texture.getUsageType()) {
            case TextureUsageType::RENDERBUFFER:
                object = new GL45AttachmentTexture(shared_from_this(), texture);
                break;

            case TextureUsageType::STRICT_RESOURCE:
                qCDebug(gpugllogging) << "Strict texture " << texture.source().c_str();
                object = new GL45StrictResourceTexture(shared_from_this(), texture);
                break;

            case TextureUsageType::RESOURCE: {
                if (GL45VariableAllocationTexture::_frameTexturesCreated < MAX_RESOURCE_TEXTURES_PER_FRAME) {
#if 0
                    if (isTextureManagementSparseEnabled() && GL45Texture::isSparseEligible(texture)) {
                        object = new GL45SparseResourceTexture(shared_from_this(), texture);
                    } else {
                        object = new GL45ResourceTexture(shared_from_this(), texture);
                    }
#else 
                    object = new GL45ResourceTexture(shared_from_this(), texture);
#endif
                    GLVariableAllocationSupport::addMemoryManagedTexture(texturePointer);
                } else {
                    auto fallback = texturePointer->getFallbackTexture();
                    if (fallback) {
                        object = static_cast<GL45Texture*>(syncGPUObject(fallback));
                    }
                }
                break;
            }

            default:
                Q_UNREACHABLE();
        }
    } else {

        if (texture.getUsageType() == TextureUsageType::RESOURCE) {
            auto varTex = static_cast<GL45VariableAllocationTexture*> (object);

            if (varTex->_minAllocatedMip > 0) {
                auto minAvailableMip = texture.minAvailableMipLevel();
                if (minAvailableMip < varTex->_minAllocatedMip) {
                    varTex->_minAllocatedMip = minAvailableMip;
                    GL45VariableAllocationTexture::_memoryPressureStateStale = true;
                }
            }
        }
    }

    return object;
}

void GL45Backend::initTextureManagementStage() {
    // enable the Sparse Texture on gl45
    _textureManagement._sparseCapable = true;

    // But now let s refine the behavior based on vendor
    std::string vendor { (const char*)glGetString(GL_VENDOR) };
    if ((vendor.find("AMD") != std::string::npos) || (vendor.find("ATI") != std::string::npos) || (vendor.find("INTEL") != std::string::npos)) {
        qCDebug(gpugllogging) << "GPU is sparse capable but force it off, vendor = " << vendor.c_str();
        _textureManagement._sparseCapable = false;
    } else {
        qCDebug(gpugllogging) << "GPU is sparse capable, vendor = " << vendor.c_str();
    }
}

using GL45Texture = GL45Backend::GL45Texture;

GL45Texture::GL45Texture(const std::weak_ptr<GLBackend>& backend, const Texture& texture)
    : GLTexture(backend, texture, allocate(texture)) {
}

GLuint GL45Texture::allocate(const Texture& texture) {
    GLuint result;
    glCreateTextures(getGLTextureType(texture), 1, &result);
#ifdef DEBUG
    auto source = texture.source();
    glObjectLabel(GL_TEXTURE, result, (GLsizei)source.length(), source.data());
#endif
    return result;
}

void GL45Texture::generateMips() const {
    glGenerateTextureMipmap(_id);
    (void)CHECK_GL_ERROR();
}

Size GL45Texture::copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum internalFormat, GLenum format, GLenum type, Size sourceSize, const void* sourcePointer) const {
    Size amountCopied = sourceSize;
    if (GL_TEXTURE_2D == _target) {
        switch (internalFormat) {
            case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_RED_RGTC1:
            case GL_COMPRESSED_RG_RGTC2:
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
                glCompressedTextureSubImage2D(_id, mip, 0, yOffset, size.x, size.y, internalFormat,
                                              static_cast<GLsizei>(sourceSize), sourcePointer);
                break;
            default:
                glTextureSubImage2D(_id, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
                break;
        }
    } else if (GL_TEXTURE_CUBE_MAP == _target) {
        switch (internalFormat) {
            case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_RED_RGTC1:
            case GL_COMPRESSED_RG_RGTC2:
            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
                if (glCompressedTextureSubImage2DEXT) {
                    auto target = GLTexture::CUBE_FACE_LAYOUT[face];
                    glCompressedTextureSubImage2DEXT(_id, target, mip, 0, yOffset, size.x, size.y, internalFormat,
                                                     static_cast<GLsizei>(sourceSize), sourcePointer);
                } else {
                    glCompressedTextureSubImage3D(_id, mip, 0, yOffset, face, size.x, size.y, 1, internalFormat,
                                                  static_cast<GLsizei>(sourceSize), sourcePointer);
                }
                break;
            default:
                // DSA ARB does not work on AMD, so use EXT
                // unless EXT is not available on the driver
                if (glTextureSubImage2DEXT) {
                    auto target = GLTexture::CUBE_FACE_LAYOUT[face];
                    glTextureSubImage2DEXT(_id, target, mip, 0, yOffset, size.x, size.y, format, type, sourcePointer);
                } else {
                    glTextureSubImage3D(_id, mip, 0, yOffset, face, size.x, size.y, 1, format, type, sourcePointer);
                }
                break;
        }
    } else {
        assert(false);
        amountCopied = 0;
    }
    (void)CHECK_GL_ERROR();

    return amountCopied;
}

void GL45Texture::syncSampler() const {
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

GL45FixedAllocationTexture::GL45FixedAllocationTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45Texture(backend, texture), _size(texture.evalTotalSize()) {
    allocateStorage();
    syncSampler();
}

GL45FixedAllocationTexture::~GL45FixedAllocationTexture() {
}

void GL45FixedAllocationTexture::allocateStorage() const {
    const GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(_gpuObject.getTexelFormat());
    const auto dimensions = _gpuObject.getDimensions();
    const auto mips = _gpuObject.getNumMips();

    glTextureStorage2D(_id, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);

    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(_id, GL_TEXTURE_MAX_LEVEL, mips - 1);
}

void GL45FixedAllocationTexture::syncSampler() const {
    Parent::syncSampler();
    const Sampler& sampler = _gpuObject.getSampler();
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), sampler.getMinMip());
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
}

// Renderbuffer attachment textures
using GL45AttachmentTexture = GL45Backend::GL45AttachmentTexture;

GL45AttachmentTexture::GL45AttachmentTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45FixedAllocationTexture(backend, texture) {
    Backend::textureFramebufferCount.increment();
    Backend::textureFramebufferGPUMemSize.update(0, size());
}

GL45AttachmentTexture::~GL45AttachmentTexture() {
    Backend::textureFramebufferCount.decrement();
    Backend::textureFramebufferGPUMemSize.update(size(), 0);
}

// Strict resource textures
using GL45StrictResourceTexture = GL45Backend::GL45StrictResourceTexture;

GL45StrictResourceTexture::GL45StrictResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45FixedAllocationTexture(backend, texture) {
    Backend::textureResidentCount.increment();
    Backend::textureResidentGPUMemSize.update(0, size());

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

GL45StrictResourceTexture::~GL45StrictResourceTexture() {
    Backend::textureResidentCount.decrement();
    Backend::textureResidentGPUMemSize.update(size(), 0);
}

