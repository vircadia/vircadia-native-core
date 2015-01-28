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
#include "GLBackendShared.h"


GLBackend::GLTexture::GLTexture() :
    _storageStamp(0),
    _contentStamp(0),
    _texture(0),
    _size(0)
{}

GLBackend::GLTexture::~GLTexture() {
    if (_texture != 0) {
        glDeleteTextures(1, &_texture);
    }
}

class GLTexelFormat {
public:
    GLenum internalFormat;
    GLenum format;
    GLenum type;

    static GLTexelFormat evalGLTexelFormat(const Element& dstFormat, const Element& srcFormat) {
        if (dstFormat != srcFormat) {
            GLTexelFormat texel = {GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE};

            switch(dstFormat.getDimension()) {
            case gpu::SCALAR: {
                texel.format = GL_RED;
                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                    texel.internalFormat = GL_RED;
                    break;
                case gpu::DEPTH:
                    texel.internalFormat = GL_DEPTH_COMPONENT;
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }
                break;
            }

            case gpu::VEC2: {
                texel.format = GL_RG;
                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                    texel.internalFormat = GL_RG;
                    break;
                case gpu::DEPTH_STENCIL:
                    texel.internalFormat = GL_DEPTH_STENCIL;
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }

                break;
            }

            case gpu::VEC3: {
                texel.format = GL_RGB;

                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                    texel.internalFormat = GL_RGB;
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }

                break;
            }

            case gpu::VEC4: {
                texel.format = GL_RGBA;
                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(srcFormat.getSemantic()) {
                case gpu::BGRA:
                case gpu::SBGRA:
                    texel.format = GL_BGRA;
                    break; 
                case gpu::RGB:
                case gpu::RGBA:
                case gpu::SRGB:
                case gpu::SRGBA:
                default:
                    break;
                };

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                    texel.internalFormat = GL_RGB;
                    break;
                case gpu::RGBA:
                    texel.internalFormat = GL_RGBA;
                    break;
                case gpu::SRGB:
                    texel.internalFormat = GL_SRGB;
                    break;
                case gpu::SRGBA:
                    texel.internalFormat = GL_SRGB_ALPHA;
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }
                break;
            }

            default:
                qDebug() << "Unknown combination of texel format";
            }
            return texel;
        } else {
            GLTexelFormat texel = {GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE};

            switch(dstFormat.getDimension()) {
            case gpu::SCALAR: {
                texel.format = GL_RED;
                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                    texel.internalFormat = GL_RED;
                    break;
                case gpu::DEPTH:
                    texel.internalFormat = GL_DEPTH_COMPONENT;
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }

                break;
            }

            case gpu::VEC2: {
                texel.format = GL_RG;
                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                    texel.internalFormat = GL_RG;
                    break;
                case gpu::DEPTH_STENCIL:
                    texel.internalFormat = GL_DEPTH_STENCIL;
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }

                break;
            }

            case gpu::VEC3: {
                texel.format = GL_RGB;

                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                case gpu::RGBA:
                    texel.internalFormat = GL_RGB;
                    break;
                case gpu::SRGB:
                case gpu::SRGBA:
                    texel.internalFormat = GL_SRGB; // standard 2.2 gamma correction color
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }
                break;
            }

            case gpu::VEC4: {
                texel.format = GL_RGBA;
                texel.type = _elementTypeToGLType[dstFormat.getType()];

                switch(dstFormat.getSemantic()) {
                case gpu::RGB:
                    texel.internalFormat = GL_RGB;
                    break;
                case gpu::RGBA:
                    texel.internalFormat = GL_RGBA;
                    break;
                case gpu::SRGB:
                    texel.internalFormat = GL_SRGB;
                    break;
                case gpu::SRGBA:
                    texel.internalFormat = GL_SRGB_ALPHA; // standard 2.2 gamma correction color
                    break;
                default:
                    qDebug() << "Unknown combination of texel format";
                }
                break;
            }

            default:
                qDebug() << "Unknown combination of texel format";
            }
            return texel;
        }
    }
};


void GLBackend::syncGPUObject(const Texture& texture) {
    GLTexture* object = Backend::getGPUObject<GLBackend::GLTexture>(texture);

    // If GPU object already created and in sync
    bool needUpdate = false;
    if (object && (object->_storageStamp == texture.getStamp())) {
        // If gpu object info is in sync with sysmem version
        if (object->_contentStamp >= texture.getDataStamp()) {
            // Then all good, GPU object is ready to be used
            return;
        } else {
            // Need to update the content of the GPU object from the source sysmem of the texture
            needUpdate = true;
        }
    } else if (!texture.isDefined()) {
        // NO texture definition yet so let's avoid thinking
        return;
    }

    // need to have a gpu object?
    if (!object) {
        object = new GLTexture();
        glGenTextures(1, &object->_texture);
        CHECK_GL_ERROR();
        Backend::setGPUObject(texture, object);
    }

    // GO through the process of allocating the correct storage and/or update the content
    switch (texture.getType()) {
    case Texture::TEX_2D: {
        if (needUpdate) {
            if (texture.isStoredMipAvailable(0)) {
                GLint boundTex = -1;
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);

                Texture::PixelsPointer mip = texture.accessStoredMip(0);
                const GLvoid* bytes = mip->_sysmem.read<Resource::Byte>();
                Element srcFormat = mip->_format;
                
                GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);

                glBindTexture(GL_TEXTURE_2D, object->_texture);
                glTexSubImage2D(GL_TEXTURE_2D, 0,
                    texelFormat.internalFormat, texture.getWidth(), texture.getHeight(), 0,
                    texelFormat.format, texelFormat.type, bytes);

                if (texture.isAutogenerateMips()) {
                    glGenerateMipmap(GL_TEXTURE_2D);
                }
                glBindTexture(GL_TEXTURE_2D, boundTex);
                object->_contentStamp = texture.getDataStamp();
            }
        } else {
            const GLvoid* bytes = 0;
            Element srcFormat = texture.getTexelFormat();
            if (texture.isStoredMipAvailable(0)) {
                Texture::PixelsPointer mip = texture.accessStoredMip(0);
                
                bytes = mip->_sysmem.read<Resource::Byte>();
                srcFormat = mip->_format;

                object->_contentStamp = texture.getDataStamp();
            }

            GLint boundTex = -1;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
            glBindTexture(GL_TEXTURE_2D, object->_texture);

            GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);
            
            glTexImage2D(GL_TEXTURE_2D, 0,
                texelFormat.internalFormat, texture.getWidth(), texture.getHeight(), 0,
                texelFormat.format, texelFormat.type, bytes);

            if (bytes && texture.isAutogenerateMips()) {
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            }

            glBindTexture(GL_TEXTURE_2D, boundTex);
            object->_storageStamp = texture.getStamp();
            object->_size = texture.getSize();
        }
        break;
    }
    default:
        qDebug() << "GLBackend::syncGPUObject(const Texture&) case for Texture Type " << texture.getType() << " not supported";	
    }
    CHECK_GL_ERROR();
}



GLuint GLBackend::getTextureID(const TexturePointer& texture) {
    if (!texture) {
        return 0;
    }
    GLBackend::syncGPUObject(*texture);
    GLTexture* object = Backend::getGPUObject<GLBackend::GLTexture>(*texture);
    if (object) {
        return object->_texture;
    } else {
        return 0;
    }
}

