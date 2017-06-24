//
//  GLBackendTexture.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 1/19/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"
#include "GLShared.h"
#include "GLFramebuffer.h"

#include <QtGui/QImage>

using namespace gpu;
using namespace gpu::gl;

void GLBackend::syncOutputStateCache() {
    GLint currentFBO;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);

    _output._drawFBO = currentFBO;
    _output._framebuffer.reset();
}

void GLBackend::resetOutputStage() {
    if (_output._framebuffer) {
        _output._framebuffer.reset();
        _output._drawFBO = 0;
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    }

    glEnable(GL_FRAMEBUFFER_SRGB);
}

void GLBackend::do_setFramebuffer(const Batch& batch, size_t paramOffset) {
    auto framebuffer = batch._framebuffers.get(batch._params[paramOffset]._uint);
    if (_output._framebuffer != framebuffer) {
        auto newFBO = getFramebufferID(framebuffer);
        if (_output._drawFBO != newFBO) {
            _output._drawFBO = newFBO;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, newFBO);
        }
        _output._framebuffer = framebuffer;
    }
}

void GLBackend::do_clearFramebuffer(const Batch& batch, size_t paramOffset) {
    if (_stereo.isStereo() && !_pipeline._stateCache.scissorEnable) {
        qWarning("Clear without scissor in stereo mode");
    }

    uint32 masks = batch._params[paramOffset + 7]._uint;
    Vec4 color;
    color.x = batch._params[paramOffset + 6]._float;
    color.y = batch._params[paramOffset + 5]._float;
    color.z = batch._params[paramOffset + 4]._float;
    color.w = batch._params[paramOffset + 3]._float;
    float depth = batch._params[paramOffset + 2]._float;
    int stencil = batch._params[paramOffset + 1]._int;
    int useScissor = batch._params[paramOffset + 0]._int;

    GLuint glmask = 0;
    bool restoreStencilMask = false;
    uint8_t cacheStencilMask = 0xFF;
    if (masks & Framebuffer::BUFFER_STENCIL) {
        glClearStencil(stencil);
        glmask |= GL_STENCIL_BUFFER_BIT;
    
        cacheStencilMask = _pipeline._stateCache.stencilActivation.getWriteMaskFront();
        if (cacheStencilMask != 0xFF) {
            restoreStencilMask = true;
            glStencilMask( 0xFF);
        }
    }

    bool restoreDepthMask = false;
    if (masks & Framebuffer::BUFFER_DEPTH) {
        glClearDepth(depth);
        glmask |= GL_DEPTH_BUFFER_BIT;
        
        bool cacheDepthMask = _pipeline._stateCache.depthTest.getWriteMask();
        if (!cacheDepthMask) {
            restoreDepthMask = true;
            glDepthMask(GL_TRUE);
        }
    }

    std::vector<GLenum> drawBuffers;
    if (masks & Framebuffer::BUFFER_COLORS) {
        if (_output._framebuffer) {
            for (unsigned int i = 0; i < Framebuffer::MAX_NUM_RENDER_BUFFERS; i++) {
                if (masks & (1 << i)) {
                    drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
                }
            }

            if (!drawBuffers.empty()) {
                glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
                glClearColor(color.x, color.y, color.z, color.w);
                glmask |= GL_COLOR_BUFFER_BIT;
            
                (void) CHECK_GL_ERROR();
            }
        } else {
            glClearColor(color.x, color.y, color.z, color.w);
            glmask |= GL_COLOR_BUFFER_BIT;
        }
        
        // Force the color mask cache to WRITE_ALL if not the case
        do_setStateColorWriteMask(State::ColorMask::WRITE_ALL);
    }

    // Apply scissor if needed and if not already on
    bool doEnableScissor = (useScissor && (!_pipeline._stateCache.scissorEnable));
    if (doEnableScissor) {
        glEnable(GL_SCISSOR_TEST);
    }

    // Clear!
    glClear(glmask);

    // Restore scissor if needed
    if (doEnableScissor) {
        glDisable(GL_SCISSOR_TEST);
    }

    // Restore Stencil write mask
    if (restoreStencilMask) {
        glStencilMask(cacheStencilMask);
    }

    // Restore write mask meaning turn back off
    if (restoreDepthMask) {
        glDepthMask(GL_FALSE);
    }
    
    // Restore the color draw buffers only if a frmaebuffer is bound
    if (_output._framebuffer && !drawBuffers.empty()) {
        auto glFramebuffer = syncGPUObject(*_output._framebuffer);
        if (glFramebuffer) {
            glDrawBuffers((GLsizei)glFramebuffer->_colorBuffers.size(), glFramebuffer->_colorBuffers.data());
        }
    }

    (void) CHECK_GL_ERROR();
}

void GLBackend::downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) {
    auto readFBO = getFramebufferID(srcFramebuffer);
    if (srcFramebuffer && readFBO) {
        if ((srcFramebuffer->getWidth() < (region.x + region.z)) || (srcFramebuffer->getHeight() < (region.y + region.w))) {
          qCWarning(gpugllogging) << "GLBackend::downloadFramebuffer : srcFramebuffer is too small to provide the region queried";
          return;
        }
    }

    if ((destImage.width() < region.z) || (destImage.height() < region.w)) {
          qCWarning(gpugllogging) << "GLBackend::downloadFramebuffer : destImage is too small to receive the region of the framebuffer";
          return;
    }

    GLenum format = GL_BGRA;
    if (destImage.format() != QImage::Format_ARGB32) {
          qCWarning(gpugllogging) << "GLBackend::downloadFramebuffer : destImage format must be FORMAT_ARGB32 to receive the region of the framebuffer";
          return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, getFramebufferID(srcFramebuffer));
    glReadPixels(region.x, region.y, region.z, region.w, format, GL_UNSIGNED_BYTE, destImage.bits());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    (void) CHECK_GL_ERROR();
}
