//
//  Created by Bradley Austin Davis on 2016/04/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLBackendTextureTransfer.h"

#include "GLBackendShared.h"

#ifdef THREADED_TEXTURE_TRANSFER

#include <gl/OffscreenGLCanvas.h>
#include <gl/QOpenGLContextWrapper.h>


#endif

using namespace gpu;

GLTextureTransferHelper::GLTextureTransferHelper() {
#ifdef THREADED_TEXTURE_TRANSFER
    _canvas = std::make_shared<OffscreenGLCanvas>();
    _canvas->create(QOpenGLContextWrapper::currentContext());
    if (!_canvas->makeCurrent()) {
        qFatal("Unable to create texture transfer context");
    }
    _canvas->doneCurrent();
    initialize(true, QThread::LowPriority);
    _canvas->moveToThreadWithContext(_thread);
#endif
}

void GLTextureTransferHelper::transferTexture(const gpu::TexturePointer& texturePointer) {
    GLBackend::GLTexture* object = Backend::getGPUObject<GLBackend::GLTexture>(*texturePointer);
#ifdef THREADED_TEXTURE_TRANSFER
    TextureTransferPackage package{ texturePointer, 0};
    object->setSyncState(GLBackend::GLTexture::Pending);
    queueItem(package);
#else
    object->transfer();
    object->setSyncState(GLBackend::GLTexture::Transferred);
#endif
}

void GLTextureTransferHelper::setup() {
#ifdef THREADED_TEXTURE_TRANSFER
    _canvas->makeCurrent();
#endif
}

void GLTextureTransferHelper::shutdown() {
    _canvas->doneCurrent();
    _canvas->moveToThreadWithContext(qApp->thread());
}


bool GLTextureTransferHelper::processQueueItems(const Queue& messages) {
    for (auto package : messages) {
        TexturePointer texturePointer = package.texture.lock();
        // Texture no longer exists, move on to the next
        if (!texturePointer) {
            continue;
        }

        GLBackend::GLTexture* object = Backend::getGPUObject<GLBackend::GLTexture>(*texturePointer);
        object->createTexture();

        object->transfer();

        object->updateSize();

        glBindTexture(object->_target, 0);
        auto writeSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glClientWaitSync(writeSync, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        glDeleteSync(writeSync);

        object->_contentStamp = texturePointer->getDataStamp();
        object->setSyncState(GLBackend::GLTexture::Transferred);
    }
    return true;
}
