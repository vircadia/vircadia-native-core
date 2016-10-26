//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLFramebuffer.h"
#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;

GLFramebuffer::~GLFramebuffer() { 
    if (_id) {
        auto backend = _backend.lock();
        if (backend) {
            backend->releaseFramebuffer(_id);
        }
    } 
}

bool GLFramebuffer::checkStatus(GLenum target) const {
    bool result = false;
    switch (_status) {
    case GL_FRAMEBUFFER_COMPLETE:
        // Success !
        result = true;
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        qCDebug(gpugllogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT.";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        qCDebug(gpugllogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT.";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        qCDebug(gpugllogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER.";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        qCDebug(gpugllogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER.";
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        qCDebug(gpugllogging) << "GLFramebuffer::syncGPUObject : Framebuffer not valid, GL_FRAMEBUFFER_UNSUPPORTED.";
        break;
    }
    return result;
}
