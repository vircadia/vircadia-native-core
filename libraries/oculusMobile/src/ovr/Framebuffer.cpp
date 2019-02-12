//
//  Created by Bradley Austin Davis on 2018/11/20
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Framebuffer.h"

#include <EGL/egl.h>
#include <glad/glad.h>
#include <android/log.h>

#include <VrApi.h>
#include <VrApi_Helpers.h>

using namespace ovr;

void Framebuffer::updateLayer(int eye, ovrLayerProjection2& layer, const ovrMatrix4f* projectionMatrix ) const {
    auto& layerTexture = layer.Textures[eye];
    layerTexture.ColorSwapChain = _swapChain;
    layerTexture.SwapChainIndex = _index;
    if (projectionMatrix) {
        layerTexture.TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection( projectionMatrix );
    }
    layerTexture.TextureRect = { 0, 0, 1, 1 };
}

void Framebuffer::create(const glm::uvec2& size) {
    _size = size;
    _index = 0;
    _validTexture = false;

    // Depth renderbuffer
    glGenRenderbuffers(1, &_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, _depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, _size.x, _size.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Framebuffer
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    _swapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, GL_RGBA8, _size.x, _size.y, 1, 3);
    _length = vrapi_GetTextureSwapChainLength(_swapChain);
    if (!_length) {
        __android_log_write(ANDROID_LOG_WARN, "QQQ_OVR", "Unable to count swap chain textures");
        return;
    }

    for (int i = 0; i < _length; ++i) {
        GLuint chainTexId = vrapi_GetTextureSwapChainHandle(_swapChain, i);
        glBindTexture(GL_TEXTURE_2D, chainTexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Framebuffer::destroy() {
    if (0 != _fbo) {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    if (0 != _depth) {
        glDeleteRenderbuffers(1, &_depth);
        _depth = 0;
    }
    if (_swapChain != nullptr) {
        vrapi_DestroyTextureSwapChain(_swapChain);
        _swapChain = nullptr;
    }
    _index = -1;
    _length = -1;
}

void Framebuffer::advance() {
    _index = (_index + 1) % _length;
    _validTexture = false;
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _fbo);
    if (!_validTexture) {
        GLuint chainTexId = vrapi_GetTextureSwapChainHandle(_swapChain, _index);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, chainTexId, 0);
        _validTexture = true;
    }
}
