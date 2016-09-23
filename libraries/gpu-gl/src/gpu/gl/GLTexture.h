//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLTexture_h
#define hifi_gpu_gl_GLTexture_h

#include "GLShared.h"
#include "GLTextureTransfer.h"
#include "GLBackend.h"
#include "GLTexelFormat.h"

namespace gpu { namespace gl {

struct GLFilterMode {
    GLint minFilter;
    GLint magFilter;
};


class GLTexture : public GLObject<Texture> {
public:
    static const uint16_t INVALID_MIP { (uint16_t)-1 };
    static const uint8_t INVALID_FACE { (uint8_t)-1 };

    static void initTextureTransferHelper();
    static std::shared_ptr<GLTextureTransferHelper> _textureTransferHelper;

    template <typename GLTextureType>
    static GLTextureType* sync(GLBackend& backend, const TexturePointer& texturePointer, bool needTransfer) {
        const Texture& texture = *texturePointer;
        if (!texture.isDefined()) {
            // NO texture definition yet so let's avoid thinking
            return nullptr;
        }

        // If the object hasn't been created, or the object definition is out of date, drop and re-create
        GLTextureType* object = Backend::getGPUObject<GLTextureType>(texture);

        // Create the texture if need be (force re-creation if the storage stamp changes
        // for easier use of immutable storage)
        if (!object || object->isInvalid()) {
            // This automatically any previous texture
            object = new GLTextureType(backend.shared_from_this(), texture, needTransfer);
            if (!object->_transferrable) {
                object->createTexture();
                object->_contentStamp = texture.getDataStamp();
                object->postTransfer();
            }
        }

        // Object maybe doens't neet to be tranasferred after creation
        if (!object->_transferrable) {
            return object;
        }

        // If we just did a transfer, return the object after doing post-transfer work
        if (GLSyncState::Transferred == object->getSyncState()) {
            object->postTransfer();
        }

        if (object->isOutdated()) {
            // Object might be outdated, if so, start the transfer
            // (outdated objects that are already in transfer will have reported 'true' for ready()
            _textureTransferHelper->transferTexture(texturePointer);
            return nullptr;
        }

        if (!object->isReady()) {
            return nullptr;
        }

        ((GLTexture*)object)->updateMips();

        return object;
    }

    template <typename GLTextureType> 
    static GLuint getId(GLBackend& backend, const TexturePointer& texture, bool shouldSync) {
        if (!texture) {
            return 0;
        }
        GLTextureType* object { nullptr };
        if (shouldSync) {
            object = sync<GLTextureType>(backend, texture, shouldSync);
        } else {
            object = Backend::getGPUObject<GLTextureType>(*texture);
        }

        if (!object) {
            return 0;
        }

        if (!shouldSync) {
            return object->_id;
        }

        // Don't return textures that are in transfer state
        if ((object->getSyncState() != GLSyncState::Idle) ||
            // Don't return transferrable textures that have never completed transfer
            (!object->_transferrable || 0 != object->_transferCount)) {
            return 0;
        }
        
        return object->_id;
    }

    ~GLTexture();

    const GLuint& _texture { _id };
    const std::string _source;
    const Stamp _storageStamp;
    const GLenum _target;
    const GLenum _internalFormat;
    const uint16 _maxMip;
    uint16 _minMip;
    const GLuint _virtualSize; // theoretical size as expected
    Stamp _contentStamp { 0 };
    const bool _transferrable;
    Size _transferCount { 0 };
    GLuint size() const { return _size; }
    GLSyncState getSyncState() const { return _syncState; }

    // Is the storage out of date relative to the gpu texture?
    bool isInvalid() const;

    // Is the content out of date relative to the gpu texture?
    bool isOutdated() const;

    // Is the texture in a state where it can be rendered with no work?
    bool isReady() const;

    // Execute any post-move operations that must occur only on the main thread
    virtual void postTransfer();

    uint16 usedMipLevels() const { return (_maxMip - _minMip) + 1; }

    static const size_t CUBE_NUM_FACES = 6;
    static const GLenum CUBE_FACE_LAYOUT[6];
    static const GLFilterMode FILTER_MODES[Sampler::NUM_FILTERS];
    static const GLenum WRAP_MODES[Sampler::NUM_WRAP_MODES];

    // Return a floating point value indicating how much of the allowed 
    // texture memory we are currently consuming.  A value of 0 indicates 
    // no texture memory usage, while a value of 1 indicates all available / allowed memory
    // is consumed.  A value above 1 indicates that there is a problem.
    static float getMemoryPressure();
protected:

    static const std::vector<GLenum>& getFaceTargets(GLenum textureType);

    static GLenum getGLTextureType(const Texture& texture);


    const GLuint _size { 0 }; // true size as reported by the gl api
    std::atomic<GLSyncState> _syncState { GLSyncState::Idle };

    GLTexture(const std::weak_ptr<gl::GLBackend>& backend, const Texture& texture, GLuint id, bool transferrable);

    void setSyncState(GLSyncState syncState) { _syncState = syncState; }

    void createTexture();

    virtual void updateMips() {}
    virtual void allocateStorage() const = 0;
    virtual void updateSize() const = 0;
    virtual void syncSampler() const = 0;
    virtual void generateMips() const = 0;
    virtual void withPreservedTexture(std::function<void()> f) const;

protected:
    void setSize(GLuint size) const;

    virtual void startTransfer();
    // Returns true if this is the last block required to complete transfer
    virtual bool continueTransfer() { return false; }
    virtual void finishTransfer();

private:
    friend class GLTextureTransferHelper;
    friend class GLBackend;
};

} }

#endif
