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

#include <gpu/GPULogging.h>

#include "GLShared.h"
#include "GLTexture.h"

#ifdef HAVE_NSIGHT
#include "nvToolsExt.h"
std::unordered_map<TexturePointer, nvtxRangeId_t> _map;
#endif


#ifdef TEXTURE_TRANSFER_PBOS
#define TEXTURE_TRANSFER_BLOCK_SIZE (64 * 1024)
#define TEXTURE_TRANSFER_PBO_COUNT 128
#endif

using namespace gpu;
using namespace gpu::gl;

GLTextureTransferHelper::GLTextureTransferHelper() {
#ifdef THREADED_TEXTURE_TRANSFER
    setObjectName("TextureTransferThread");
    _context.create();
    initialize(true, QThread::LowPriority);
    // Clean shutdown on UNIX, otherwise _canvas is freed early
    connect(qApp, &QCoreApplication::aboutToQuit, [&] { terminate(); });
#else
    initialize(false, QThread::LowPriority);
#endif
}

GLTextureTransferHelper::~GLTextureTransferHelper() {
#ifdef THREADED_TEXTURE_TRANSFER
    if (isStillRunning()) {
        terminate();
    }
#else
    terminate();
#endif
}

void GLTextureTransferHelper::transferTexture(const gpu::TexturePointer& texturePointer) {
    GLTexture* object = Backend::getGPUObject<GLTexture>(*texturePointer);

    Backend::incrementTextureGPUTransferCount();
    object->setSyncState(GLSyncState::Pending);
    Lock lock(_mutex);
    _pendingTextures.push_back(texturePointer);
}

void GLTextureTransferHelper::setup() {
#ifdef THREADED_TEXTURE_TRANSFER
    _context.makeCurrent();

#ifdef TEXTURE_TRANSFER_FORCE_DRAW
    // FIXME don't use opengl 4.5 DSA functionality without verifying it's present
    glCreateRenderbuffers(1, &_drawRenderbuffer);
    glNamedRenderbufferStorage(_drawRenderbuffer, GL_RGBA8, 128, 128);
    glCreateFramebuffers(1, &_drawFramebuffer);
    glNamedFramebufferRenderbuffer(_drawFramebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _drawRenderbuffer);
    glCreateFramebuffers(1, &_readFramebuffer);
#endif

#ifdef TEXTURE_TRANSFER_PBOS
    std::array<GLuint, TEXTURE_TRANSFER_PBO_COUNT> pbos;
    glCreateBuffers(TEXTURE_TRANSFER_PBO_COUNT, &pbos[0]);
    for (uint32_t i = 0; i < TEXTURE_TRANSFER_PBO_COUNT; ++i) {
        TextureTransferBlock newBlock;
        newBlock._pbo = pbos[i];
        glNamedBufferStorage(newBlock._pbo, TEXTURE_TRANSFER_BLOCK_SIZE, 0, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        newBlock._mapped = glMapNamedBufferRange(newBlock._pbo, 0, TEXTURE_TRANSFER_BLOCK_SIZE, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
        _readyQueue.push(newBlock);
    }
#endif
#endif
}

void GLTextureTransferHelper::shutdown() {
#ifdef THREADED_TEXTURE_TRANSFER
    _context.makeCurrent();
#endif

#ifdef TEXTURE_TRANSFER_FORCE_DRAW
    glNamedFramebufferRenderbuffer(_drawFramebuffer, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, 0);
    glDeleteFramebuffers(1, &_drawFramebuffer);
    _drawFramebuffer = 0;
    glDeleteFramebuffers(1, &_readFramebuffer);
    _readFramebuffer = 0;

    glNamedFramebufferTexture(_readFramebuffer, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0);
    glDeleteRenderbuffers(1, &_drawRenderbuffer);
    _drawRenderbuffer = 0;
#endif
}

void GLTextureTransferHelper::queueExecution(VoidLambda lambda) {
    Lock lock(_mutex);
    _pendingCommands.push_back(lambda);
}

#define MAX_TRANSFERS_PER_PASS 2

bool GLTextureTransferHelper::process() {
    // Take any new textures or commands off the queue
    VoidLambdaList pendingCommands;
    TextureList newTransferTextures;
    {
        Lock lock(_mutex);
        newTransferTextures.swap(_pendingTextures);
        pendingCommands.swap(_pendingCommands);
    }

    if (!pendingCommands.empty()) {
        for (auto command : pendingCommands) {
            command();
        }
        glFlush();
    }

    if (!newTransferTextures.empty()) {
        for (auto& texturePointer : newTransferTextures) {
#ifdef HAVE_NSIGHT
            _map[texturePointer] = nvtxRangeStart("TextureTansfer");
#endif
            GLTexture* object = Backend::getGPUObject<GLTexture>(*texturePointer);
            object->startTransfer();
            _transferringTextures.push_back(texturePointer);
            _textureIterator = _transferringTextures.begin();
        }
        _transferringTextures.sort([](const gpu::TexturePointer& a, const gpu::TexturePointer& b)->bool {
            return a->getSize() < b->getSize();
        });
    }

    // No transfers in progress, sleep
    if (_transferringTextures.empty()) {
#ifdef THREADED_TEXTURE_TRANSFER
        QThread::usleep(1);
#endif
        return true;
    }

    static auto lastReport = usecTimestampNow();
    auto now = usecTimestampNow();
    auto lastReportInterval = now - lastReport;
    if (lastReportInterval > USECS_PER_SECOND * 4) {
        lastReport = now;
        qCDebug(gpulogging) << "Texture list " << _transferringTextures.size();
    }

    size_t transferCount = 0;
    for (_textureIterator = _transferringTextures.begin(); _textureIterator != _transferringTextures.end();) {
        if (++transferCount > MAX_TRANSFERS_PER_PASS) {
            break;
        }
        auto texture = *_textureIterator;
        GLTexture* gltexture = Backend::getGPUObject<GLTexture>(*texture);
        if (gltexture->continueTransfer()) {
            ++_textureIterator;
            continue;
        }

        gltexture->finishTransfer();

#ifdef TEXTURE_TRANSFER_FORCE_DRAW
        // FIXME force a draw on the texture transfer thread before passing the texture to the main thread for use
#endif

#ifdef THREADED_TEXTURE_TRANSFER
        clientWait();
#endif
        gltexture->_contentStamp = gltexture->_gpuObject.getDataStamp();
        gltexture->updateSize();
        gltexture->setSyncState(gpu::gl::GLSyncState::Transferred);
        Backend::decrementTextureGPUTransferCount();
#ifdef HAVE_NSIGHT
        // Mark the texture as transferred
        nvtxRangeEnd(_map[texture]);
        _map.erase(texture);
#endif
        _textureIterator = _transferringTextures.erase(_textureIterator);
    }

#ifdef THREADED_TEXTURE_TRANSFER
    if (!_transferringTextures.empty()) {
        // Don't saturate the GPU
        clientWait();
    } else {
        // Don't saturate the CPU
        QThread::msleep(1);
    }
#endif

    return true;
}
