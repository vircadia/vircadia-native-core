//
//  Created by Bradley Austin Davis on 2015/08/08
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OculusHelpers.h"

// A wrapper for constructing and using a swap texture set,
// where each frame you draw to a texture via the FBO,
// then submit it and increment to the next texture.
// The Oculus SDK manages the creation and destruction of
// the textures

SwapFramebufferWrapper::SwapFramebufferWrapper(const ovrSession& session) 
    : _session(session) {
    color = nullptr;
    depth = nullptr;
}

SwapFramebufferWrapper::~SwapFramebufferWrapper() {
    destroyColor();
}

void SwapFramebufferWrapper::Increment() {
    ++color->CurrentIndex;
    color->CurrentIndex %= color->TextureCount;
}

void SwapFramebufferWrapper::Resize(const uvec2 & size) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oglplus::GetName(fbo));
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    this->size = size;
    initColor();
    initDone();
}

void SwapFramebufferWrapper::destroyColor() {
    if (color) {
        ovr_DestroySwapTextureSet(_session, color);
        color = nullptr;
    }
}

void SwapFramebufferWrapper::initColor() {
    destroyColor();

    if (!OVR_SUCCESS(ovr_CreateSwapTextureSetGL(_session, GL_SRGB8_ALPHA8, size.x, size.y, &color))) {
        qFatal("Unable to create swap textures");
    }

    for (int i = 0; i < color->TextureCount; ++i) {
        ovrGLTexture& ovrTex = (ovrGLTexture&)color->Textures[i];
        glBindTexture(GL_TEXTURE_2D, ovrTex.OGL.TexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void SwapFramebufferWrapper::initDone() {
}

void SwapFramebufferWrapper::onBind(oglplus::Framebuffer::Target target) {
    ovrGLTexture& tex = (ovrGLTexture&)(color->Textures[color->CurrentIndex]);
    glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.OGL.TexId, 0);
}

void SwapFramebufferWrapper::onUnbind(oglplus::Framebuffer::Target target) {
    glFramebufferTexture2D(toEnum(target), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
}
