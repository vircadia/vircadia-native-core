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
#include <QtCore/QThread>

#include <NumericalConstants.h>
#include "../gl/GLTexelFormat.h"

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl45;

#define SPARSE_PAGE_SIZE_OVERHEAD_ESTIMATE 1.3f

GLTexture* GL45Backend::syncGPUObject(const TexturePointer& texturePointer) {
    if (!texturePointer) {
        return nullptr;
    }

    const Texture& texture = *texturePointer;
    if (std::string("cursor texture") == texture.source()) {
        qDebug() << "Loading cursor texture";
    }
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

                GL45VariableAllocationTexture* varObject { nullptr };
#if 0
                if (isTextureManagementSparseEnabled() && GL45Texture::isSparseEligible(texture)) {
                    varObject = new GL45SparseResourceTexture(shared_from_this(), texture);
                } else {
                    varObject = new GL45ResourceTexture(shared_from_this(), texture);
                }
#else 
                varObject = new GL45ResourceTexture(shared_from_this(), texture);
#endif
                GL45VariableAllocationTexture::addMemoryManagedTexture(texturePointer);
                object = varObject;
                break;
            }

            default:
                Q_UNREACHABLE();
        }
    }

    return object;
}

void GL45Backend::recycle() const {
    Parent::recycle();
    GL45VariableAllocationTexture::manageMemory();
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
    return result;
}

void GL45Texture::generateMips() const {
    glGenerateTextureMipmap(_id);
    (void)CHECK_GL_ERROR();
}

void GL45Texture::copyMipFromTexture(uint16_t sourceMip, uint16_t targetMip) const {
    const auto& texture = _gpuObject;
    if (!texture.isStoredMipFaceAvailable(sourceMip)) {
        return;
    }
    size_t maxFace = GLTexture::getFaceCount(_target);
    for (uint8_t face = 0; face < maxFace; ++face) {
        auto size = texture.evalMipDimensions(sourceMip);
        auto mipData = texture.accessStoredMipFace(sourceMip, face);
        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), mipData->getFormat());
        if (GL_TEXTURE_2D == _target) {
            glTextureSubImage2D(_id, targetMip, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mipData->readData());
        } else if (GL_TEXTURE_CUBE_MAP == _target) {
            // DSA ARB does not work on AMD, so use EXT
            // unless EXT is not available on the driver
            if (glTextureSubImage2DEXT) {
                auto target = GLTexture::CUBE_FACE_LAYOUT[face];
                glTextureSubImage2DEXT(_id, target, targetMip, 0, 0, size.x, size.y, texelFormat.format, texelFormat.type, mipData->readData());
            } else {
                glTextureSubImage3D(_id, targetMip, 0, 0, face, size.x, size.y, 1, texelFormat.format, texelFormat.type, mipData->readData());
            }
        } else {
            Q_ASSERT(false);
        }
        (void)CHECK_GL_ERROR();
    }
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

#if 0
    // FIXME account for mip offsets here
    auto baseMip = std::max<uint16_t>(sampler.getMipOffset(), _minMip);
    glTextureParameteri(_id, GL_TEXTURE_BASE_LEVEL, baseMip);
    glTextureParameterf(_id, GL_TEXTURE_MIN_LOD, (float)sampler.getMinMip());
    glTextureParameterf(_id, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip() - _mipOffset));
#endif
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
    const auto mips = _gpuObject.evalNumMips();
    glTextureStorage2D(_id, mips, texelFormat.internalFormat, dimensions.x, dimensions.y);
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
    Backend::updateTextureGPUFramebufferMemoryUsage(0, size());
}

GL45AttachmentTexture::~GL45AttachmentTexture() {
    Backend::updateTextureGPUFramebufferMemoryUsage(size(), 0);
}

// Strict resource textures
using GL45StrictResourceTexture = GL45Backend::GL45StrictResourceTexture;

GL45StrictResourceTexture::GL45StrictResourceTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture) : GL45FixedAllocationTexture(backend, texture) {
    auto mipLevels = _gpuObject.evalNumMips();
    for (uint16_t sourceMip = 0; sourceMip < mipLevels; ++sourceMip) {
        uint16_t targetMip = sourceMip;
        copyMipFromTexture(sourceMip, targetMip);
    }
    if (texture.isAutogenerateMips()) {
        generateMips();
    }
}

