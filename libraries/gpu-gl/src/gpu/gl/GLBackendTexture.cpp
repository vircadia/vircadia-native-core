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
#include "GLBackend.h"
#include "GLTexture.h"

using namespace gpu;
using namespace gpu::gl;

bool GLBackend::isTextureReady(const TexturePointer& texture) const {
    // DO not transfer the texture, this call is expected for rendering texture
    GLTexture* object = syncGPUObject(texture, true);
    return object && object->isReady();
}


void GLBackend::do_generateTextureMips(Batch& batch, size_t paramOffset) {
    TexturePointer resourceTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);
    if (!resourceTexture) {
        return;
    }

    // DO not transfer the texture, this call is expected for rendering texture
    GLTexture* object = syncGPUObject(resourceTexture, false);
    if (!object) {
        return;
    }

    object->generateMips();
}
