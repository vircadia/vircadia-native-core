//
//  Created by Bradley Austin Davis on 2016/04/03
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QSharedPointer>
#include <GenericQueueThread.h>
#include "GLBackendShared.h"

#define THREADED_TEXTURE_TRANSFER

class OffscreenGLCanvas;

namespace gpu {

struct TextureTransferPackage {
    std::weak_ptr<Texture> texture;
    GLsync fence;
};

class GLTextureTransferHelper : public GenericQueueThread<TextureTransferPackage> {
public:
    GLTextureTransferHelper();
    ~GLTextureTransferHelper();
    void transferTexture(const gpu::TexturePointer& texturePointer);
    void postTransfer(const gpu::TexturePointer& texturePointer);

protected:
    void setup() override;
    void shutdown() override;
    bool processQueueItems(const Queue& messages) override;
    void transferTextureSynchronous(const gpu::Texture& texture);

private:
    QSharedPointer<OffscreenGLCanvas> _canvas;
};

template <typename F>
void withPreservedTexture(GLenum target, F f) {
    GLint boundTex = -1;
    switch (target) {
    case GL_TEXTURE_2D:
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
        break;

    case GL_TEXTURE_CUBE_MAP:
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundTex);
        break;

    default:
        qFatal("Unsupported texture type");
    }
    (void)CHECK_GL_ERROR();

    f();

    glBindTexture(target, boundTex);
    (void)CHECK_GL_ERROR();
}

}
