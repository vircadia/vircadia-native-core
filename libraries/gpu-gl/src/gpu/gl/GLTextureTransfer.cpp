//
//  Created by Bradley Austin Davis on 2016/04/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLTextureTransfer.h"

#include <gl/GLHelpers.h>
#include <gl/Context.h>

#include "GLShared.h"
#include "GLTexture.h"

using namespace gpu;
using namespace gpu::gl;

GLTextureTransferHelper::GLTextureTransferHelper() {
#ifdef THREADED_TEXTURE_TRANSFER
    setObjectName("TextureTransferThread");
    _context.create();
    initialize(true, QThread::LowPriority);
    // Clean shutdown on UNIX, otherwise _canvas is freed early
    connect(qApp, &QCoreApplication::aboutToQuit, [&] { terminate(); });
#endif
}

GLTextureTransferHelper::~GLTextureTransferHelper() {
#ifdef THREADED_TEXTURE_TRANSFER
    if (isStillRunning()) {
        terminate();
    }
#endif
}

void GLTextureTransferHelper::transferTexture(const gpu::TexturePointer& texturePointer) {
    GLTexture* object = Backend::getGPUObject<GLTexture>(*texturePointer);
    Backend::incrementTextureGPUTransferCount();
#ifdef THREADED_TEXTURE_TRANSFER
    GLsync fence { 0 };
    //fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    //glFlush();

    TextureTransferPackage package { texturePointer, fence };
    object->setSyncState(GLSyncState::Pending);
    queueItem(package);
#else
    object->withPreservedTexture([&] {
        do_transfer(*object);
    });
    object->_contentStamp = texturePointer->getDataStamp();
    object->setSyncState(GLSyncState::Transferred);
#endif
}

void GLTextureTransferHelper::setup() {
}

void GLTextureTransferHelper::shutdown() {
}

void GLTextureTransferHelper::do_transfer(GLTexture& texture) {
    texture.createTexture();
    texture.transfer();
    texture.updateSize();
    Backend::decrementTextureGPUTransferCount();
}

bool GLTextureTransferHelper::processQueueItems(const Queue& messages) {
#ifdef THREADED_TEXTURE_TRANSFER
    _context.makeCurrent();
#endif
    for (auto package : messages) {
        TexturePointer texturePointer = package.texture.lock();
        // Texture no longer exists, move on to the next
        if (!texturePointer) {
            continue;
        }

        if (package.fence) {
            auto result = glClientWaitSync(package.fence, 0, 0);
            while (GL_TIMEOUT_EXPIRED == result || GL_WAIT_FAILED == result) {
                // Minimum sleep
                QThread::usleep(1);
                result = glClientWaitSync(package.fence, 0, 0);
            }
            assert(GL_CONDITION_SATISFIED == result || GL_ALREADY_SIGNALED == result);
            glDeleteSync(package.fence);
            package.fence = 0;
        }

        GLTexture* object = Backend::getGPUObject<GLTexture>(*texturePointer);

        do_transfer(*object);
        glBindTexture(object->_target, 0);

        {
            auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            assert(fence);
            auto result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            while (GL_TIMEOUT_EXPIRED == result || GL_WAIT_FAILED == result) {
                // Minimum sleep
                QThread::usleep(1);
                result = glClientWaitSync(fence, 0, 0);
            }
            glDeleteSync(package.fence);
        }

        object->_contentStamp = texturePointer->getDataStamp();
        object->setSyncState(GLSyncState::Transferred);
    }
#ifdef THREADED_TEXTURE_TRANSFER
    _context.doneCurrent();
#endif
    return true;
}
