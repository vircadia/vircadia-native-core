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
#include "../gl/GLFramebuffer.h"
#include "../gl/GLTexture.h"

#include <QtGui/QImage>

namespace gpu { namespace gl45 { 

class GL45Framebuffer : public gl::GLFramebuffer {
    using Parent = gl::GLFramebuffer;
    static GLuint allocate() {
        GLuint result;
        glCreateFramebuffers(1, &result);
        return result;
    }
public:
    void update() override {
        gl::GLTexture* gltexture = nullptr;
        TexturePointer surface;
        if (_gpuObject.getColorStamps() != _colorStamps) {
            if (_gpuObject.hasColor()) {
                _colorBuffers.clear();
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
                auto backend = _backend.lock();
                for (auto& b : _gpuObject.getRenderBuffers()) {
                    surface = b._texture;
                    if (surface) {
                        Q_ASSERT(TextureUsageType::RENDERBUFFER == surface->getUsageType());
                        gltexture = backend->syncGPUObject(surface);
                    } else {
                        gltexture = nullptr;
                    }

                    if (gltexture) {
                        glNamedFramebufferTexture(_id, colorAttachments[unit], gltexture->_texture, 0);
                        _colorBuffers.push_back(colorAttachments[unit]);
                    } else {
                        glNamedFramebufferTexture(_id, colorAttachments[unit], 0, 0);
                    }
                    unit++;
                }
            }
            _colorStamps = _gpuObject.getColorStamps();
        }

        GLenum attachement = GL_DEPTH_STENCIL_ATTACHMENT;
        if (!_gpuObject.hasStencil()) {
            attachement = GL_DEPTH_ATTACHMENT;
        } else if (!_gpuObject.hasDepth()) {
            attachement = GL_STENCIL_ATTACHMENT;
        }

        if (_gpuObject.getDepthStamp() != _depthStamp) {
            auto surface = _gpuObject.getDepthStencilBuffer();
            auto backend = _backend.lock();
            if (_gpuObject.hasDepthStencil() && surface) {
                Q_ASSERT(TextureUsageType::RENDERBUFFER == surface->getUsageType());
                gltexture = backend->syncGPUObject(surface);
            }

            if (gltexture) {
                glNamedFramebufferTexture(_id, attachement, gltexture->_texture, 0);
            } else {
                glNamedFramebufferTexture(_id, attachement, 0, 0);
            }
            _depthStamp = _gpuObject.getDepthStamp();
        }


        // Last but not least, define where we draw
        if (!_colorBuffers.empty()) {
            glNamedFramebufferDrawBuffers(_id, (GLsizei)_colorBuffers.size(), _colorBuffers.data());
        } else {
            glNamedFramebufferDrawBuffer(_id, GL_NONE);
        }

        // Now check for completness
        _status = glCheckNamedFramebufferStatus(_id, GL_DRAW_FRAMEBUFFER);

        // restore the current framebuffer
        checkStatus();
    }


public:
    GL45Framebuffer(const std::weak_ptr<gl::GLBackend>& backend, const gpu::Framebuffer& framebuffer)
        : Parent(backend, framebuffer, allocate()) { }
};

gl::GLFramebuffer* GL45Backend::syncGPUObject(const Framebuffer& framebuffer) {
    return gl::GLFramebuffer::sync<GL45Framebuffer>(*this, framebuffer);
}

GLuint GL45Backend::getFramebufferID(const FramebufferPointer& framebuffer) {
    return framebuffer ? gl::GLFramebuffer::getId<GL45Framebuffer>(*this, *framebuffer) : 0;
}

void GL45Backend::do_blit(const Batch& batch, size_t paramOffset) {
    auto srcframebuffer = batch._framebuffers.get(batch._params[paramOffset]._uint);
    Vec4i srcvp;
    for (auto i = 0; i < 4; ++i) {
        srcvp[i] = batch._params[paramOffset + 1 + i]._int;
    }

    auto dstframebuffer = batch._framebuffers.get(batch._params[paramOffset + 5]._uint);
    Vec4i dstvp;
    for (auto i = 0; i < 4; ++i) {
        dstvp[i] = batch._params[paramOffset + 6 + i]._int;
    }

    // Assign dest framebuffer if not bound already
    auto destFbo = getFramebufferID(dstframebuffer);
    auto srcFbo = getFramebufferID(srcframebuffer);
    glBlitNamedFramebuffer(srcFbo, destFbo,
        srcvp.x, srcvp.y, srcvp.z, srcvp.w,
        dstvp.x, dstvp.y, dstvp.z, dstvp.w,
        GL_COLOR_BUFFER_BIT, GL_LINEAR);
    (void) CHECK_GL_ERROR();
}

} }
