//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLTexture.h"

#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;


const GLenum GLTexture::CUBE_FACE_LAYOUT[GLTexture::TEXTURE_CUBE_NUM_FACES] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
};

const GLenum GLTexture::WRAP_MODES[Sampler::NUM_WRAP_MODES] = {
    GL_REPEAT,                         // WRAP_REPEAT,
    GL_MIRRORED_REPEAT,                // WRAP_MIRROR,
    GL_CLAMP_TO_EDGE,                  // WRAP_CLAMP,
    GL_CLAMP_TO_BORDER,                // WRAP_BORDER,
    GL_MIRROR_CLAMP_TO_EDGE_EXT        // WRAP_MIRROR_ONCE,
};     

const GLFilterMode GLTexture::FILTER_MODES[Sampler::NUM_FILTERS] = {
    { GL_NEAREST, GL_NEAREST },  //FILTER_MIN_MAG_POINT,
    { GL_NEAREST, GL_LINEAR },  //FILTER_MIN_POINT_MAG_LINEAR,
    { GL_LINEAR, GL_NEAREST },  //FILTER_MIN_LINEAR_MAG_POINT,
    { GL_LINEAR, GL_LINEAR },  //FILTER_MIN_MAG_LINEAR,

    { GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST },  //FILTER_MIN_MAG_MIP_POINT,
    { GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST },  //FILTER_MIN_MAG_POINT_MIP_LINEAR,
    { GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR },  //FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
    { GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR },  //FILTER_MIN_POINT_MAG_MIP_LINEAR,
    { GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST },  //FILTER_MIN_LINEAR_MAG_MIP_POINT,
    { GL_LINEAR_MIPMAP_LINEAR, GL_NEAREST },  //FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
    { GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR },  //FILTER_MIN_MAG_LINEAR_MIP_POINT,
    { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR },  //FILTER_MIN_MAG_MIP_LINEAR,
    { GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR }  //FILTER_ANISOTROPIC,
};

GLenum GLTexture::getGLTextureType(const Texture& texture) {
    switch (texture.getType()) {
    case Texture::TEX_2D:
        return GL_TEXTURE_2D;
        break;

    case Texture::TEX_CUBE:
        return GL_TEXTURE_CUBE_MAP;
        break;

    default:
        qFatal("Unsupported texture type");
    }
    Q_UNREACHABLE();
    return GL_TEXTURE_2D;
}


uint8_t GLTexture::getFaceCount(GLenum target) {
    switch (target) {
        case GL_TEXTURE_2D:
            return TEXTURE_2D_NUM_FACES;
        case GL_TEXTURE_CUBE_MAP:
            return TEXTURE_CUBE_NUM_FACES;
        default:
            Q_UNREACHABLE();
            break;
    }
}
const std::vector<GLenum>& GLTexture::getFaceTargets(GLenum target) {
    static std::vector<GLenum> cubeFaceTargets {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    static std::vector<GLenum> faceTargets {
        GL_TEXTURE_2D
    };
    switch (target) {
    case GL_TEXTURE_2D:
        return faceTargets;
    case GL_TEXTURE_CUBE_MAP:
        return cubeFaceTargets;
    default:
        Q_UNREACHABLE();
        break;
    }
    Q_UNREACHABLE();
    return faceTargets;
}

GLTexture::GLTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint id) :
    GLObject(backend, texture, id),
    _source(texture.source()),
    _target(getGLTextureType(texture))
{
    Backend::setGPUObject(texture, this);
}

GLTexture::~GLTexture() {
    auto backend = _backend.lock();
    if (backend && _id) {
        backend->releaseTexture(_id, 0);
    }
}


GLExternalTexture::GLExternalTexture(const std::weak_ptr<GLBackend>& backend, const Texture& texture, GLuint id) 
    : Parent(backend, texture, id) { }

GLExternalTexture::~GLExternalTexture() {
    auto backend = _backend.lock();
    if (backend) {
        auto recycler = _gpuObject.getExternalRecycler();
        if (recycler) {
            backend->releaseExternalTexture(_id, recycler);
        } else {
            qWarning() << "No recycler available for texture " << _id << " possible leak";
        }
        const_cast<GLuint&>(_id) = 0;
    }
}
