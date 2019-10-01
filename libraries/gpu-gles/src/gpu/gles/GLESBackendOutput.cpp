//
//  GLESBackendOutput.cpp
//  libraries/gpu-gl-android/src/gpu/gles
//
//  Created by Gabriel Calero & Cristian Duarte on 9/27/2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLESBackend.h"

#include <QtGui/QImage>

#include <gpu/gl/GLFramebuffer.h>
#include <gpu/gl/GLTexture.h>

namespace gpu { namespace gles { 

class GLESFramebuffer : public gl::GLFramebuffer {
    using Parent = gl::GLFramebuffer;
    static GLuint allocate() {
        GLuint result;
        glGenFramebuffers(1, &result);
        return result;
    }
public:
    void update() override {
        GLint currentFBO = -1;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

        vec2 focalPoint{ -1.0f };

#if 0
        {
            auto backend = _backend.lock();
            if (backend && backend->isStereo()) {
                glm::mat4 projections[2];
                backend->getStereoProjections(projections);
                vec4 fov = extractFov(projections[0]);
                float fovwidth = fov.x + fov.y;
                float fovheight = fov.z + fov.w;
                focalPoint.x = fov.y / fovwidth;
                focalPoint.y = (fov.z / fovheight) - 0.5f;
            }
        }
#endif

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
                        if (gltexture->_target == GL_TEXTURE_2D) {
                            glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[unit], GL_TEXTURE_2D, gltexture->_texture, 0);
#if 0
                            if (glTextureFoveationParametersQCOM && focalPoint.x != -1.0f) {
                                static GLint FOVEATION_QUERY = 0;
                                static std::once_flag once;
                                std::call_once(once, [&]{
                                    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_FOVEATED_FEATURE_QUERY_QCOM, &FOVEATION_QUERY);
                                });
                                static const float foveaArea = 4.0f;
                                static const float gain = 16.0f;
                                GLESBackend::GLESTexture* glestexture = static_cast<GLESBackend::GLESTexture*>(gltexture);
                                glestexture->withPreservedTexture([=]{
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_FOVEATED_FEATURE_BITS_QCOM, GL_FOVEATION_ENABLE_BIT_QCOM | GL_FOVEATION_SCALED_BIN_METHOD_BIT_QCOM);
                                    glTextureFoveationParametersQCOM(_id, 0, 0, -focalPoint.x, focalPoint.y, gain * 2.0f, gain, foveaArea);
                                    glTextureFoveationParametersQCOM(_id, 0, 1, focalPoint.x, focalPoint.y, gain * 2.0f, gain, foveaArea);
                                });

                            }
#endif
                        } else if (gltexture->_target == GL_TEXTURE_2D_MULTISAMPLE) {
                            glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[unit], GL_TEXTURE_2D_MULTISAMPLE, gltexture->_texture, 0);
                        } else {
                            glFramebufferTextureLayer(GL_FRAMEBUFFER, colorAttachments[unit], gltexture->_texture, 0,
                                                      b._subresource);
                        }
                        _colorBuffers.push_back(colorAttachments[unit]);
                    } else {
                        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[unit], GL_TEXTURE_2D, 0, 0);
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
            auto backend = _backend.lock();
            auto surface = _gpuObject.getDepthStencilBuffer();
            if (_gpuObject.hasDepthStencil() && surface) {
                Q_ASSERT(TextureUsageType::RENDERBUFFER == surface->getUsageType());
                gltexture = backend->syncGPUObject(surface);
            }

            if (gltexture) {
                if (gltexture->_target == GL_TEXTURE_2D) {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, attachement, GL_TEXTURE_2D, gltexture->_texture, 0);
                } else if (gltexture->_target == GL_TEXTURE_2D_MULTISAMPLE) {
                    glFramebufferTexture2D(GL_FRAMEBUFFER, attachement, GL_TEXTURE_2D_MULTISAMPLE, gltexture->_texture, 0);
                } else {
                    glFramebufferTextureLayer(GL_FRAMEBUFFER, attachement, gltexture->_texture, 0,
                                              _gpuObject.getDepthStencilBufferSubresource());
                }
            } else {
                glFramebufferTexture2D(GL_FRAMEBUFFER, attachement, GL_TEXTURE_2D, 0, 0);
            }
            _depthStamp = _gpuObject.getDepthStamp();
        }


        // Last but not least, define where we draw
        if (!_colorBuffers.empty()) {
            glDrawBuffers((GLsizei)_colorBuffers.size(), _colorBuffers.data());
        } else {
            GLenum DrawBuffers[1] = {GL_NONE};
            glDrawBuffers(1, DrawBuffers);
        }

        // Now check for completness
        _status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        // restore the current framebuffer
        if (currentFBO != -1) {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentFBO);
        }

        checkStatus();
    }


public:
    GLESFramebuffer(const std::weak_ptr<gl::GLBackend>& backend, const gpu::Framebuffer& framebuffer)
        : Parent(backend, framebuffer, allocate()) { }
};

gl::GLFramebuffer* GLESBackend::syncGPUObject(const Framebuffer& framebuffer) {
    return GLESFramebuffer::sync<GLESFramebuffer>(*this, framebuffer);
}

GLuint GLESBackend::getFramebufferID(const FramebufferPointer& framebuffer) {
    return framebuffer ? GLESFramebuffer::getId<GLESFramebuffer>(*this, *framebuffer) : 0;
}

void GLESBackend::do_blit(const Batch& batch, size_t paramOffset) {
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


} }
