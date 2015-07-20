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
#endif
        

        if (framebuffer.hasDepthStencil()) {
            auto surface = framebuffer.getDepthStencilBuffer();
            if (surface) {
                auto gltexture = GLBackend::syncGPUObject(*surface);
                if (gltexture) {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gltexture->_texture, 0);
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

void GLBackend::do_setFramebuffer(Batch& batch, uint32 paramOffset) {
    auto framebuffer = batch._framebuffers.get(batch._params[paramOffset]._uint);

    if (_output._framebuffer != framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getFramebufferID(framebuffer));
        _output._framebuffer = framebuffer;
    }
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
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, getFramebufferID(dstframebuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, getFramebufferID(srcframebuffer));
    glBlitFramebuffer(srcvp.x, srcvp.y, srcvp.z, srcvp.w, 
        dstvp.x, dstvp.y, dstvp.z, dstvp.w,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
}
