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


GLuint GLBackend::getTextureID(const TexturePointer& texture) {
    GLTexture* object = syncGPUObject(texture);

    if (!object) {
        return 0;
    }

    return object->_id;
}

GLTexture* GLBackend::syncGPUObject(const TexturePointer& texturePointer) {
    const Texture& texture = *texturePointer;
    // Special case external textures
    if (TextureUsageType::EXTERNAL == texture.getUsageType()) {
        Texture::ExternalUpdates updates = texture.getUpdates();
        if (!updates.empty()) {
            Texture::ExternalRecycler recycler = texture.getExternalRecycler();
            Q_ASSERT(recycler);
            // Discard any superfluous updates
            while (updates.size() > 1) {
                const auto& update = updates.front();
                // Superfluous updates will never have been read, but we want to ensure the previous 
                // writes to them are complete before they're written again, so return them with the 
                // same fences they arrived with.  This can happen on any thread because no GL context 
                // work is involved
                recycler(update.first, update.second);
                updates.pop_front();
            }

            // The last texture remaining is the one we'll use to create the GLTexture
            const auto& update = updates.front();
            // Check for a fence, and if it exists, inject a wait into the command stream, then destroy the fence
            if (update.second) {
                GLsync fence = static_cast<GLsync>(update.second);
                glWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
                glDeleteSync(fence);
            }

            // Create the new texture object (replaces any previous texture object)
            new GLExternalTexture(shared_from_this(), texture, update.first);
        }

        // Return the texture object (if any) associated with the texture, without extensive logic
        // (external textures are 
        return Backend::getGPUObject<GLTexture>(texture);
    }

    return nullptr;
}

void GLBackend::do_generateTextureMips(const Batch& batch, size_t paramOffset) {
    const auto& resourceTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);
    if (!resourceTexture) {
        return;
    }

    // DO not transfer the texture, this call is expected for rendering texture
    GLTexture* object = syncGPUObject(resourceTexture);
    if (!object) {
        return;
    }

    object->generateMips();
}
