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
#include <qimage.h>

#include "GPULogging.h"
#include "GLBackendShared.h"


using namespace gpu;

GLBackend::GLFramebuffer::GLFramebuffer() {}

GLBackend::GLFramebuffer::~GLFramebuffer() {
    if (_fbo != 0) {
        glDeleteFramebuffers(1, &_fbo);
    }
}

GLBackend::GLFramebuffer* GLBackend::syncGPUObject(const Framebuffer& framebuffer) {
    GLFramebuffer* object = Backend::getGPUObject<GLBackend::GLFramebuffer>(framebuffer);

    // If GPU object already created and in sync
    if (object) {
        return object;
    } else if (framebuffer.isEmpty()) {
        // NO framebuffer definition yet so let's avoid thinking
        return nullptr;
    }

    // need to have a gpu object?
    if (!object) {
        GLint currentFBO;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);
        
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        (void) CHECK_GL_ERROR();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        std::vector<GLenum> colorBuffers;
        if (framebuffer.hasColor()) {
            static const GLenum colorAttachments[] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3,
                GL_COLOR_ATTACHMENT4,
                GL_COLOR_ATTACHMENT5,
                GL_COLOR_ATTACHMENT6,
                GL_COLOR_ATTACHMENT7,
                GL_COLOR_ATTACHMENT8,
                GL_COLOR_ATTACHMENT9,
                GL_COLOR_ATTACHMENT10,
                GL_COLOR_ATTACHMENT11,
                GL_COLOR_ATTACHMENT12,
                GL_COLOR_ATTACHMENT13,
                GL_COLOR_ATTACHMENT14,
                GL_COLOR_ATTACHMENT15 };

            int unit = 0;
            for (auto& b : framebuffer.getRenderBuffers()) {
                auto surface = b._texture;
                if (surface) {
                    auto gltexture = GLBackend::syncGPUObject(*surface);
                    if (gltexture) {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[unit], GL_TEXTURE_2D, gltexture->_texture, 0);
                    }
                    colorBuffers.push_back(colorAttachments[unit]);
                    unit++;
                }
            }
        }
#if (GPU_FEATURE_PROFILE == GPU_LEGACY)
        // for reasons that i don't understand yet, it seems that on mac gl, a fbo must have a color buffer...
        else {
            GLuint renderBuffer = 0;
            glGenRenderbuffers(1, &renderBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, framebuffer.getWidth(), framebuffer.getHeight());
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderBuffer);
            (void) CHECK_GL_ERROR();
        }
        
   //     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
#endif
        

        if (framebuffer.hasDepthStencil()) {
            auto surface = framebuffer.getDepthStencilBuffer();
            if (surface) {
                auto gltexture = GLBackend::syncGPUObject(*surface);
                if (gltexture) {
                    GLenum attachement = GL_DEPTH_STENCIL_ATTACHMENT;
                    if (!framebuffer.hasStencil()) {
                        attachement = GL_DEPTH_ATTACHMENT;
                        glFramebufferTexture2D(GL_FRAMEBUFFER, attachement, GL_TEXTURE_2D, gltexture->_texture, 0);
                    } else if (!framebuffer.hasDepth()) {
                        attachement = GL_STENCIL_ATTACHMENT;
                        glFramebufferTexture2D(GL_FRAMEBUFFER, attachement, GL_TEXTURE_2D, gltexture->_texture, 0);
                    } else {
                        attachement = GL_DEPTH_STENCIL_ATTACHMENT;
                        glFramebufferTexture2D(GL_FRAMEBUFFER, attachement, GL_TEXTURE_2D, gltexture->_texture, 0);
                    }
                    (void) CHECK_GL_ERROR();
                }
            }
        }

        // Last but not least, define where we draw
        if (!colorBuffers.empty()) {
            glDrawBuffers(colorBuffers.size(), colorBuffers.data());
        } else {
            glDrawBuffer( GL_NONE );
        }

        // Now check for completness
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        bool result = false;
        switch (status) {
        case GL_FRAMEBUFFER_COMPLETE :
            // Success !
            result = true;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT :
            qCDebug(gpulogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT.";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT :
            qCDebug(gpulogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT.";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER :
            qCDebug(gpulogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER.";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER :
            qCDebug(gpulogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER.";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED :
            qCDebug(gpulogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_UNSUPPORTED.";
            break;
        }
        if (!result && fbo) {
            glDeleteFramebuffers( 1, &fbo );
            return nullptr;
        }


        // All is green, assign the gpuobject to the Framebuffer
        object = new GLFramebuffer();
        object->_fbo = fbo;
        object->_colorBuffers = colorBuffers;
        Backend::setGPUObject(framebuffer, object);
        
        // restore the current framebuffer
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFBO);
    }

    return object;
}



GLuint GLBackend::getFramebufferID(const FramebufferPointer& framebuffer) {
    if (!framebuffer) {
        return 0;
    }
    GLFramebuffer* object = GLBackend::syncGPUObject(*framebuffer);
    if (object) {
        return object->_fbo;
    } else {
        return 0;
    }
}

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
}

void GLBackend::do_setFramebuffer(Batch& batch, uint32 paramOffset) {
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

void GLBackend::do_clearFramebuffer(Batch& batch, uint32 paramOffset) {
    if (_stereo._enable && !_pipeline._stateCache.scissorEnable) {
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
    if (masks & Framebuffer::BUFFER_STENCIL) {
        glClearStencil(stencil);
        glmask |= GL_STENCIL_BUFFER_BIT;
        // TODO: we will probably need to also check the write mask of stencil like we do
        // for depth buffer, but as would say a famous Fez owner "We'll cross that bridge when we come to it"
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
                glDrawBuffers(drawBuffers.size(), drawBuffers.data());
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

    // Restore write mask meaning turn back off
    if (restoreDepthMask) {
        glDepthMask(GL_FALSE);
    }
    
    // Restore the color draw buffers only if a frmaebuffer is bound
    if (_output._framebuffer && !drawBuffers.empty()) {
        auto glFramebuffer = syncGPUObject(*_output._framebuffer);
        if (glFramebuffer) {
            glDrawBuffers(glFramebuffer->_colorBuffers.size(), glFramebuffer->_colorBuffers.data());
        }
    }

    (void) CHECK_GL_ERROR();
}

void GLBackend::do_blit(Batch& batch, uint32 paramOffset) {
    auto srcframebuffer = batch._framebuffers.get(batch._params[paramOffset]._uint);
    Vec4i srcvp;
    for (size_t i = 0; i < 4; ++i) {
        srcvp[i] = batch._params[paramOffset + 1 + i]._int;
    }

    auto dstframebuffer = batch._framebuffers.get(batch._params[paramOffset + 5]._uint);
    Vec4i dstvp;
    for (size_t i = 0; i < 4; ++i) {
        dstvp[i] = batch._params[paramOffset + 6 + i]._int;
    }

    // Assign dest framebuffer if not bound already
    auto newDrawFBO = getFramebufferID(dstframebuffer);
    if (_output._drawFBO != newDrawFBO) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, newDrawFBO);
    }

    // always bind the read fbo
    glBindFramebuffer(GL_READ_FRAMEBUFFER, getFramebufferID(srcframebuffer));

    // Blit!
    glBlitFramebuffer(srcvp.x, srcvp.y, srcvp.z, srcvp.w, 
        dstvp.x, dstvp.y, dstvp.z, dstvp.w,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Always clean the read fbo to 0
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Restore draw fbo if changed
    if (_output._drawFBO != newDrawFBO) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _output._drawFBO);
    }

    (void) CHECK_GL_ERROR();
}

void GLBackend::downloadFramebuffer(const FramebufferPointer& srcFramebuffer, const Vec4i& region, QImage& destImage) {
    auto readFBO = gpu::GLBackend::getFramebufferID(srcFramebuffer);
    if (srcFramebuffer && readFBO) {
        if ((srcFramebuffer->getWidth() < (region.x + region.z)) || (srcFramebuffer->getHeight() < (region.y + region.w))) {
          qCDebug(gpulogging) << "GLBackend::downloadFramebuffer : srcFramebuffer is too small to provide the region queried";
          return;
        }
    }

    if ((destImage.width() < region.z) || (destImage.height() < region.w)) {
          qCDebug(gpulogging) << "GLBackend::downloadFramebuffer : destImage is too small to receive the region of the framebuffer";
          return;
    }

    GLenum format = GL_BGRA;
    if (destImage.format() != QImage::Format_ARGB32) {
          qCDebug(gpulogging) << "GLBackend::downloadFramebuffer : destImage format must be FORMAT_ARGB32 to receive the region of the framebuffer";
          return;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(srcFramebuffer));
    glReadPixels(region.x, region.y, region.z, region.w, format, GL_UNSIGNED_BYTE, destImage.bits());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    (void) CHECK_GL_ERROR();
}