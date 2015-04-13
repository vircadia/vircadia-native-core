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


GLBackend::GLFramebuffer::GLFramebuffer()
{}

GLBackend::GLFramebuffer::~GLFramebuffer() {
    if (_fbo != 0) {
        glDeleteFramebuffers(1, &_fbo);
    }
}

GLBackend::GLFramebuffer* GLBackend::syncGPUObject(const Framebuffer& framebuffer) {
    GLFramebuffer* object = Backend::getGPUObject<GLBackend::GLFramebuffer>(framebuffer);

    // If GPU object already created and in sync
    bool needUpdate = false;
    if (object) {
        return object;
    } else if (!framebuffer.isDefined()) {
        // NO framebuffer definition yet so let's avoid thinking
        return nullptr;
    }

    // need to have a gpu object?
    if (!object) {
        object = new GLFramebuffer();
        glGenFramebuffers(1, &object->_fbo);
        CHECK_GL_ERROR();
        Backend::setGPUObject(framebuffer, object);
    }


    CHECK_GL_ERROR();

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

