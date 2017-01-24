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
#include "GLBackend.h"
#include "GLTexelFormat.h"

namespace gpu { namespace gl {

struct GLFilterMode {
    GLint minFilter;
    GLint magFilter;
};

class GLTexture : public GLObject<Texture> {
    using Parent = GLObject<Texture>;
    friend class GLBackend;
public:
    static const uint16_t INVALID_MIP { (uint16_t)-1 };
    static const uint8_t INVALID_FACE { (uint8_t)-1 };

    ~GLTexture();

    const GLuint& _texture { _id };
    const std::string _source;
    const GLenum _target;

    static const std::vector<GLenum>& getFaceTargets(GLenum textureType);
    static uint8_t getFaceCount(GLenum textureType);
    static GLenum getGLTextureType(const Texture& texture);

    static const uint8_t TEXTURE_2D_NUM_FACES = 1;
    static const uint8_t TEXTURE_CUBE_NUM_FACES = 6;
    static const GLenum CUBE_FACE_LAYOUT[TEXTURE_CUBE_NUM_FACES];
    static const GLFilterMode FILTER_MODES[Sampler::NUM_FILTERS];
    static const GLenum WRAP_MODES[Sampler::NUM_WRAP_MODES];

protected:
    virtual uint32 size() const = 0;
    virtual void generateMips() const = 0;

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
    uint32 size() const override { return 0; }
};


} }

#endif
