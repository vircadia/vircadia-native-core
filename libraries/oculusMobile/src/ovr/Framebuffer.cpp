//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Framebuffer.h"

#include <array>

#include <EGL/egl.h>
#include <android/log.h>

#include <VrApi.h>
#include <VrApi_Helpers.h>

#include "Helpers.h"

using namespace ovr;

void Framebuffer::updateLayer(int eye, ovrLayerProjection2& layer, const ovrMatrix4f* projectionMatrix ) const {
    auto& layerTexture = layer.Textures[eye];
    layerTexture.ColorSwapChain = _swapChainInfos[eye].swapChain;
    layerTexture.SwapChainIndex = _swapChainInfos[eye].index;
    if (projectionMatrix) {
        layerTexture.TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection( projectionMatrix );
    }
    layerTexture.TextureRect = { 0, 0, 1, 1 };
}

void Framebuffer::SwapChainInfo::destroy() {
    if (swapChain != nullptr) {
        vrapi_DestroyTextureSwapChain(swapChain);
        swapChain = nullptr;
    }
    index = -1;
    length = -1;
}

void Framebuffer::create(const glm::uvec2& size) {
    _size = size;
    ovr::for_each_eye([&](ovrEye eye) {
        _swapChainInfos[eye].create(size);
    });
    glBindTexture(GL_TEXTURE_2D, 0);
    glGenFramebuffers(1, &_fbo);
}

void Framebuffer::destroy() {
    if (0 != _fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }

    ovr::for_each_eye([&](ovrEye eye) {
        _swapChainInfos[eye].destroy();
    });
}

void Framebuffer::advance() {
    ovr::for_each_eye([&](ovrEye eye) {
        _swapChainInfos[eye].advance();
    });
}

void Framebuffer::bind(GLenum target) {
    glBindFramebuffer(target, _fbo);
    _swapChainInfos[0].bind(target, GL_COLOR_ATTACHMENT0);
    _swapChainInfos[1].bind(target, GL_COLOR_ATTACHMENT1);
}

void Framebuffer::invalidate(GLenum target) {
    static const std::array<GLenum, 2> INVALIDATE_ATTACHMENTS {{ GL_COLOR_ATTACHMENT0,  GL_COLOR_ATTACHMENT1 }};
    glInvalidateFramebuffer(target, static_cast<GLsizei>(INVALIDATE_ATTACHMENTS.size()), INVALIDATE_ATTACHMENTS.data());
}


void Framebuffer::drawBuffers(ovrEye eye) const {
    static const std::array<std::array<GLenum, 2>, 3> EYE_DRAW_BUFFERS { {
        {GL_COLOR_ATTACHMENT0, GL_NONE},
        {GL_NONE, GL_COLOR_ATTACHMENT1},
        {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1}
    } };

    switch(eye) {
        case VRAPI_EYE_LEFT:
        case VRAPI_EYE_RIGHT:
        case VRAPI_EYE_COUNT: {
                const auto& eyeDrawBuffers = EYE_DRAW_BUFFERS[eye];
                glDrawBuffers(static_cast<GLsizei>(eyeDrawBuffers.size()), eyeDrawBuffers.data());
            }
            break;

        default:
            throw std::runtime_error("Invalid eye for drawBuffers");
    }
}

void Framebuffer::SwapChainInfo::create(const glm::uvec2 &size) {
    index = 0;
    validTexture = false;
    // GL_SRGB8_ALPHA8 and GL_RGBA8 appear to behave the same here.  The only thing that changes the
    // output gamma behavior is VRAPI_MODE_FLAG_FRONT_BUFFER_SRGB passed to vrapi_EnterVrMode
    swapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, GL_SRGB8_ALPHA8, size.x, size.y, 1, 3);
    length = vrapi_GetTextureSwapChainLength(swapChain);
    if (!length) {
        __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "Unable to count swap chain textures");
        throw std::runtime_error("Unable to create Oculus texture swap chain");
    }

    for (int i = 0; i < length; ++i) {
        GLuint chainTexId = vrapi_GetTextureSwapChainHandle(swapChain, i);
        glBindTexture(GL_TEXTURE_2D, chainTexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

void Framebuffer::SwapChainInfo::advance() {
    index = (index + 1) % length;
    validTexture = false;
}

void Framebuffer::SwapChainInfo::bind(uint32_t target, uint32_t attachment) {
    if (!validTexture) {
        GLuint chainTexId = vrapi_GetTextureSwapChainHandle(swapChain, index);
        glFramebufferTexture(target, attachment, chainTexId, 0);
        validTexture = true;
    }
}
