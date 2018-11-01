//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLTexture_h
#define hifi_gpu_gl_GLTexture_h

#include <QtCore/QThreadPool>
#include <QtConcurrent>

#include "GLShared.h"
#include "GLBackend.h"
#include "GLTexelFormat.h"
#include <thread>

namespace gpu { namespace gl {

struct GLFilterMode {
    GLint minFilter;
    GLint magFilter;
};

class GLTextureTransferEngine {
public:
    using Pointer = std::shared_ptr<GLTextureTransferEngine>;

    virtual ~GLTextureTransferEngine() = default;

    /// Called once per frame to perform any require memory management or transfer work
    virtual void manageMemory() = 0;
    virtual void shutdown() = 0;

    /// Called whenever a client wants to create a new texture.  This allows the transfer engine to 
    /// potentially limit the number of GL textures created per frame
    bool allowCreate() const { return _frameTexturesCreated < MAX_RESOURCE_TEXTURES_PER_FRAME; }
    /// Called whenever a client creates a new resource texture that should use managed memory
    /// and incremental transfer
    void addMemoryManagedTexture(const TexturePointer& texturePointer);

protected:
    // Fetch all the currently active textures as strong pointers, while clearing the 
    // empty weak pointers out of _registeredTextures
    std::vector<TexturePointer> getAllTextures();
    void resetFrameTextureCreated() { _frameTexturesCreated = 0;  }

private:
    static const size_t MAX_RESOURCE_TEXTURES_PER_FRAME{ 2 };
    size_t _frameTexturesCreated{ 0 };
    std::list<TextureWeakPointer> _registeredTextures;
};

/**
  A transfer job encapsulates an individual piece of work required to upload texture data to the GPU.  
  The work can be broken down into two parts, expressed as lambdas.  The buffering lambda is repsonsible
  for putting the data to be uploaded into a CPU memory buffer.  The transfer lambda is repsonsible for 
  uploading the data from the CPU memory buffer to the GPU using OpenGL calls.  Ideally the buffering lambda 
  will be executed on a seprate thread from the OpenGL work to ensure that disk IO operations do not block 
  OpenGL calls

  Additionally, a TransferJob can encapsulate some kind of post-upload work that changes the state of the 
  GLTexture derived object wrapping the actual texture ID, such as changing the _populateMip value once
  a given mip level has been compeltely uploaded
 */
class TransferJob {
public:
    using Pointer = std::shared_ptr<TransferJob>;
    using Queue = std::queue<Pointer>;
    using Lambda = std::function<void(const TexturePointer&)>;
private:
    Texture::PixelsPointer _mipData;
    size_t _transferOffset{ 0 };
    size_t _transferSize{ 0 };
    uint16_t _sourceMip{ 0 };
    bool _bufferingRequired{ true };
    Lambda _transferLambda{ [](const TexturePointer&) {} };
    Lambda _bufferingLambda{ [](const TexturePointer&) {} };
public:
    TransferJob(const TransferJob& other) = delete;
    TransferJob(uint16_t sourceMip, const std::function<void()>& transferLambda);
    TransferJob(const Texture& texture, uint16_t sourceMip, uint16_t targetMip, uint8_t face, uint32_t lines = 0, uint32_t lineOffset = 0);
    ~TransferJob();
    const uint16_t& sourceMip() const { return _sourceMip; }
    const size_t& size() const { return _transferSize; }
    bool bufferingRequired() const { return _bufferingRequired; }
    void buffer(const TexturePointer& texture) { _bufferingLambda(texture); }
    void transfer(const TexturePointer& texture) { _transferLambda(texture); }
};

using TransferJobPointer = std::shared_ptr<TransferJob>;
using TransferQueue = std::queue<TransferJobPointer>;

class GLVariableAllocationSupport {
    friend class GLBackend;

public:
    GLVariableAllocationSupport();
    virtual ~GLVariableAllocationSupport();
    virtual void populateTransferQueue(TransferQueue& pendingTransfers) = 0;

    void sanityCheck() const;
    uint16 populatedMip() const { return _populatedMip; }
    bool canPromote() const { return _allocatedMip > _minAllocatedMip; }
    bool canDemote() const { return _allocatedMip < _maxAllocatedMip; }
    bool hasPendingTransfers() const { return _populatedMip > _allocatedMip; }

    virtual size_t promote() = 0;
    virtual size_t demote() = 0;

    static const uvec3 MAX_TRANSFER_DIMENSIONS;
    static const uvec3 INITIAL_MIP_TRANSFER_DIMENSIONS;
    static const size_t MAX_TRANSFER_SIZE;
    static const size_t MAX_BUFFER_SIZE;

protected:
    // THe amount of memory currently allocated
    Size _size { 0 };

    // The amount of memory currnently populated
    void incrementPopulatedSize(Size delta) const;
    void decrementPopulatedSize(Size delta) const;
    mutable Size _populatedSize { 0 };

    // The allocated mip level, relative to the number of mips in the gpu::Texture object 
    // The relationship between a given glMip to the original gpu::Texture mip is always 
    // glMip + _allocatedMip
    uint16 _allocatedMip { 0 };
    // The populated mip level, relative to the number of mips in the gpu::Texture object
    // This must always be >= the allocated mip
    uint16 _populatedMip { 0 };
    // The highest (lowest resolution) mip that we will support, relative to the number 
    // of mips in the gpu::Texture object
    uint16 _maxAllocatedMip { 0 };
    // The lowest (highest resolution) mip that we will support, relative to the number
    // of mips in the gpu::Texture object
    uint16 _minAllocatedMip { 0 };
};

class GLTexture : public GLObject<Texture> {
    using Parent = GLObject<Texture>;
    friend class GLBackend;
    friend class GLVariableAllocationSupport;
public:
    static const uint16_t INVALID_MIP { (uint16_t)-1 };
    static const uint8_t INVALID_FACE { (uint8_t)-1 };

    ~GLTexture();

    const GLuint& _texture { _id };
    const std::string _source;
    const GLenum _target;
    GLTexelFormat _texelFormat;

    static const std::vector<GLenum>& getFaceTargets(GLenum textureType);
    static uint8_t getFaceCount(GLenum textureType);
    static GLenum getGLTextureType(const Texture& texture);
    virtual Size size() const = 0;
    virtual Size copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum internalFormat, GLenum format, GLenum type, Size sourceSize, const void* sourcePointer) const = 0;
    virtual Size copyMipFaceFromTexture(uint16_t sourceMip, uint16_t targetMip, uint8_t face) const final;

    static const uint8_t TEXTURE_2D_NUM_FACES = 1;
    static const uint8_t TEXTURE_CUBE_NUM_FACES = 6;
    static const GLenum CUBE_FACE_LAYOUT[TEXTURE_CUBE_NUM_FACES];
    static const GLFilterMode FILTER_MODES[Sampler::NUM_FILTERS];
    static const GLenum WRAP_MODES[Sampler::NUM_WRAP_MODES];

protected:
    virtual void generateMips() const = 0;
    virtual void syncSampler() const = 0;

    virtual void copyTextureMipsInGPUMem(GLuint srcId, GLuint destId, uint16_t srcMipOffset, uint16_t destMipOffset, uint16_t populatedMips) {} // Only relevant for Variable Allocation textures

    GLTexture(const std::weak_ptr<gl::GLBackend>& backend, const Texture& texture, GLuint id);
};

class GLExternalTexture : public GLTexture {
    using Parent = GLTexture;
    friend class GLBackend;
public:
    ~GLExternalTexture();
protected:
    GLExternalTexture(const std::weak_ptr<gl::GLBackend>& backend, const Texture& texture, GLuint id);
    void generateMips() const override {}
    void syncSampler() const override {}
    Size copyMipFaceLinesFromTexture(uint16_t mip, uint8_t face, const uvec3& size, uint32_t yOffset, GLenum internalFormat, GLenum format, GLenum type, Size sourceSize, const void* sourcePointer) const override { return 0;}

    Size size() const override { return 0; }
};

} }

#endif
