//
//  Created by Bradley Austin Davis on 2016/04/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLTextureTransfer_h
#define hifi_gpu_gl_GLTextureTransfer_h

#include <QtGlobal>
#include <QtCore/QSharedPointer>

#include <GenericQueueThread.h>

#include <gl/Context.h>

#include "GLShared.h"

#ifdef Q_OS_WIN
#define THREADED_TEXTURE_TRANSFER
#endif

#ifdef THREADED_TEXTURE_TRANSFER
// FIXME when sparse textures are enabled, it's harder to force a draw on the transfer thread
// also, the current draw code is implicitly using OpenGL 4.5 functionality
//#define TEXTURE_TRANSFER_FORCE_DRAW
// FIXME PBO's increase the complexity and don't seem to work reliably
//#define TEXTURE_TRANSFER_PBOS
#endif

namespace gpu { namespace gl {

using TextureList = std::list<TexturePointer>;
using TextureListIterator = TextureList::iterator;

class GLTextureTransferHelper : public GenericThread {
public:
    using VoidLambda = std::function<void()>;
    using VoidLambdaList = std::list<VoidLambda>;
    using Pointer = std::shared_ptr<GLTextureTransferHelper>;
    GLTextureTransferHelper();
    ~GLTextureTransferHelper();
    void transferTexture(const gpu::TexturePointer& texturePointer);
    void queueExecution(VoidLambda lambda);

    void setup() override;
    void shutdown() override;
    bool process() override;

private:
#ifdef THREADED_TEXTURE_TRANSFER
    ::gl::OffscreenContext _context;
#endif

#ifdef TEXTURE_TRANSFER_FORCE_DRAW
    // Framebuffers / renderbuffers for forcing access to the texture on the transfer thread
    GLuint _drawRenderbuffer { 0 };
    GLuint _drawFramebuffer { 0 };
    GLuint _readFramebuffer { 0 };
#endif

    // A mutex for protecting items access on the render and transfer threads
    Mutex _mutex;
    // Commands that have been submitted for execution on the texture transfer thread
    VoidLambdaList _pendingCommands;
    // Textures that have been submitted for transfer
    TextureList _pendingTextures;
    // Textures currently in the transfer process
    // Only used on the transfer thread
    TextureList _transferringTextures;
    TextureListIterator _textureIterator;

};

} }

#endif