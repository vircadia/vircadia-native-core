//
//  Created by Bradley Austin Davis on 2016/04/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLBackendTextureTransfer.h"

#include "GLBackendShared.h"
#include "GLTexelFormat.h"

#ifdef THREADED_TEXTURE_TRANSFER

#include <gl/OffscreenGLCanvas.h>
#include <gl/QOpenGLContextWrapper.h>

//#define FORCE_DRAW_AFTER_TRANSFER

#ifdef FORCE_DRAW_AFTER_TRANSFER

#include <gl/OglplusHelpers.h>

static ProgramPtr _program;
static ProgramPtr _cubeProgram;
static ShapeWrapperPtr _plane;
static ShapeWrapperPtr _skybox;
static BasicFramebufferWrapperPtr _framebuffer;

#endif

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
    TextureTransferPackage package { texturePointer, glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0) };
    glFlush();
    object->setSyncState(GLBackend::GLTexture::Pending);
    queueItem(package);
#else
    object->transfer();
    object->postTransfer();
#endif
}

void GLTextureTransferHelper::setup() {
#ifdef THREADED_TEXTURE_TRANSFER
    _canvas->makeCurrent();

#ifdef FORCE_DRAW_AFTER_TRANSFER
    _program = loadDefaultShader();
    _plane = loadPlane(_program);
    _cubeProgram = loadCubemapShader();
    _skybox = loadSkybox(_cubeProgram);
    _framebuffer = std::make_shared<BasicFramebufferWrapper>();
    _framebuffer->Init({ 100, 100 });
    _framebuffer->fbo.Bind(oglplus::FramebufferTarget::Draw);
#endif

#endif
}

void GLTextureTransferHelper::shutdown() {
    _canvas->doneCurrent();
    _canvas->moveToThreadWithContext(qApp->thread());
}


bool GLTextureTransferHelper::processQueueItems(const Queue& messages) {
    for (auto package : messages) {
        glWaitSync(package.fence, 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(package.fence);
        TexturePointer texturePointer = package.texture.lock();
        // Texture no longer exists, move on to the next
        if (!texturePointer) {
            continue;
        }

        GLBackend::GLTexture* object = Backend::getGPUObject<GLBackend::GLTexture>(*texturePointer);
        object->transfer();

#ifdef FORCE_DRAW_AFTER_TRANSFER        
        // Now force a draw using the texture
        try {
            switch (texturePointer->getType()) {
                case Texture::TEX_2D:
                    _program->Use();
                    _plane->Use();
                    _plane->Draw();
                    break;

                case Texture::TEX_CUBE:
                    _cubeProgram->Use();
                    _skybox->Use();
                    _skybox->Draw();
                    break;

                default:
                    qCWarning(gpulogging) << __FUNCTION__ << " case for Texture Type " << texturePointer->getType() << " not supported";
                    break;
            }
        } catch (const std::runtime_error& error) {
            qWarning() << "Failed to render texture on background thread: " << error.what();
        }
#endif

        glBindTexture(object->_target, 0);
        auto writeSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        glClientWaitSync(writeSync, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        glDeleteSync(writeSync);
        object->_contentStamp = texturePointer->getDataStamp();
        object->setSyncState(GLBackend::GLTexture::Transferred);
    }
    return true;
}
