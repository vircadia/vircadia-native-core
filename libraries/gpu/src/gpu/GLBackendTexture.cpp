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
#include "GPULogging.h"
#include "GLBackendShared.h"

using namespace gpu;

GLBackend::GLTexture::GLTexture() :
    _storageStamp(0),
    _contentStamp(0),
    _texture(0),
    _target(GL_TEXTURE_2D),
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
                    qCDebug(gpulogging) << "Unknown combination of texel format";
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
                    qCDebug(gpulogging) << "Unknown combination of texel format";
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
                    qCDebug(gpulogging) << "Unknown combination of texel format";
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
                    qCDebug(gpulogging) << "Unknown combination of texel format";
                }
                break;
            }

            default:
                qCDebug(gpulogging) << "Unknown combination of texel format";
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
                    texel.format = GL_DEPTH_COMPONENT; // It's depth component to load it
                    texel.internalFormat = GL_DEPTH_COMPONENT;
                    switch (dstFormat.getType()) {
                    case gpu::UINT32:
                    case gpu::INT32:
                    case gpu::NUINT32:
                    case gpu::NINT32: {
                        texel.internalFormat = GL_DEPTH_COMPONENT32;
                        break;
                        }
                    case gpu::FLOAT: {
                        texel.internalFormat = GL_DEPTH_COMPONENT32F;
                        break;
                        }
                    case gpu::UINT16:
                    case gpu::INT16:
                    case gpu::NUINT16:
                    case gpu::NINT16:
                    case gpu::HALF: {
                        texel.internalFormat = GL_DEPTH_COMPONENT16;
                        break;
                        }
                    case gpu::UINT8:
                    case gpu::INT8:
                    case gpu::NUINT8:
                    case gpu::NINT8: {
                        texel.internalFormat = GL_DEPTH_COMPONENT24;
                        break;
                        }
                    case gpu::NUM_TYPES: { // quiet compiler
                        Q_UNREACHABLE();
                    }
                    }
                    break;
                default:
                    qCDebug(gpulogging) << "Unknown combination of texel format";
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
                    qCDebug(gpulogging) << "Unknown combination of texel format";
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
                    qCDebug(gpulogging) << "Unknown combination of texel format";
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
                    qCDebug(gpulogging) << "Unknown combination of texel format";
                }
                break;
            }

            default:
                qCDebug(gpulogging) << "Unknown combination of texel format";
            }
            return texel;
        }
    }
};


GLBackend::GLTexture* GLBackend::syncGPUObject(const Texture& texture) {
    GLTexture* object = Backend::getGPUObject<GLBackend::GLTexture>(texture);

    // If GPU object already created and in sync
    bool needUpdate = false;
    if (object && (object->_storageStamp == texture.getStamp())) {
        // If gpu object info is in sync with sysmem version
        if (object->_contentStamp >= texture.getDataStamp()) {
            // Then all good, GPU object is ready to be used
            return object;
        } else {
            // Need to update the content of the GPU object from the source sysmem of the texture
            needUpdate = true;
        }
    } else if (!texture.isDefined()) {
        // NO texture definition yet so let's avoid thinking
        return nullptr;
    }

    // need to have a gpu object?
    if (!object) {
        object = new GLTexture();
        glGenTextures(1, &object->_texture);
        (void) CHECK_GL_ERROR();
        Backend::setGPUObject(texture, object);
    }

    // GO through the process of allocating the correct storage and/or update the content
    switch (texture.getType()) {
    case Texture::TEX_2D: {
        if (texture.getNumSlices() == 1) {
            GLint boundTex = -1;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
            glBindTexture(GL_TEXTURE_2D, object->_texture);

            if (needUpdate) {
                if (texture.isStoredMipFaceAvailable(0)) {
                    Texture::PixelsPointer mip = texture.accessStoredMipFace(0);
                    const GLvoid* bytes = mip->_sysmem.read<Byte>();
                    Element srcFormat = mip->_format;
                
                    GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);

                    glBindTexture(GL_TEXTURE_2D, object->_texture);
                    glTexSubImage2D(GL_TEXTURE_2D, 0,
                        texelFormat.internalFormat, texture.getWidth(), texture.getHeight(), 0,
                        texelFormat.format, texelFormat.type, bytes);

                    if (texture.isAutogenerateMips()) {
                        glGenerateMipmap(GL_TEXTURE_2D);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    }

                object->_target = GL_TEXTURE_2D;

                syncSampler(texture.getSampler(), texture.getType(), object);


                    // At this point the mip piels have been loaded, we can notify
                    texture.notifyMipFaceGPULoaded(0, 0);

                    object->_contentStamp = texture.getDataStamp();
                }
            } else {
                const GLvoid* bytes = 0;
                Element srcFormat = texture.getTexelFormat();
                if (texture.isStoredMipFaceAvailable(0)) {
                    Texture::PixelsPointer mip = texture.accessStoredMipFace(0);
                
                    bytes = mip->_sysmem.read<Byte>();
                    srcFormat = mip->_format;

                    object->_contentStamp = texture.getDataStamp();
                }

                GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);
            
                glTexImage2D(GL_TEXTURE_2D, 0,
                    texelFormat.internalFormat, texture.getWidth(), texture.getHeight(), 0,
                    texelFormat.format, texelFormat.type, bytes);

                if (bytes && texture.isAutogenerateMips()) {
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                }/* else {
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }*/
                
                object->_target = GL_TEXTURE_2D;

                syncSampler(texture.getSampler(), texture.getType(), object);

                // At this point the mip pixels have been loaded, we can notify
                texture.notifyMipFaceGPULoaded(0, 0);

                object->_storageStamp = texture.getStamp();
                object->_contentStamp = texture.getDataStamp();
                object->_size = texture.getSize();
            }

            glBindTexture(GL_TEXTURE_2D, boundTex);
        }
        break;
    }
    case Texture::TEX_CUBE: {
        if (texture.getNumSlices() == 1) {
            GLint boundTex = -1;
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundTex);
            glBindTexture(GL_TEXTURE_CUBE_MAP, object->_texture);
            const int NUM_FACES = 6;
            const GLenum FACE_LAYOUT[] = {
                 GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                 GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                 GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };
            if (needUpdate) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, object->_texture);

                // transfer pixels from each faces
                for (int f = 0; f < NUM_FACES; f++) {
                    if (texture.isStoredMipFaceAvailable(0, f)) {
                        Texture::PixelsPointer mipFace = texture.accessStoredMipFace(0, f);
                        Element srcFormat = mipFace->_format;
                        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);

                        glTexSubImage2D(FACE_LAYOUT[f], 0, texelFormat.internalFormat, texture.getWidth(), texture.getWidth(), 0,
                                texelFormat.format, texelFormat.type, (GLvoid*) (mipFace->_sysmem.read<Byte>()));

                        // At this point the mip pixels have been loaded, we can notify
                        texture.notifyMipFaceGPULoaded(0, f);
                    }
                }

                if (texture.isAutogenerateMips()) {
                    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                }

                object->_target = GL_TEXTURE_CUBE_MAP;

                syncSampler(texture.getSampler(), texture.getType(), object);

                object->_contentStamp = texture.getDataStamp();

            } else {
                glBindTexture(GL_TEXTURE_CUBE_MAP, object->_texture);

                // transfer pixels from each faces
                for (int f = 0; f < NUM_FACES; f++) {
                    if (texture.isStoredMipFaceAvailable(0, f)) {
                        Texture::PixelsPointer mipFace = texture.accessStoredMipFace(0, f);
                        Element srcFormat = mipFace->_format;
                        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);

                        glTexImage2D(FACE_LAYOUT[f], 0, texelFormat.internalFormat, texture.getWidth(), texture.getWidth(), 0,
                                texelFormat.format, texelFormat.type, (GLvoid*) (mipFace->_sysmem.read<Byte>()));

                        // At this point the mip pixels have been loaded, we can notify
                        texture.notifyMipFaceGPULoaded(0, f);
                    }
                }

                if (texture.isAutogenerateMips()) {
                    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                } else {
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                }
                
                object->_target = GL_TEXTURE_CUBE_MAP;

                syncSampler(texture.getSampler(), texture.getType(), object);

                object->_storageStamp = texture.getStamp();
                object->_contentStamp = texture.getDataStamp();
                object->_size = texture.getSize();
            }

            glBindTexture(GL_TEXTURE_CUBE_MAP, boundTex);
        }
        break;
    }
    default:
        qCDebug(gpulogging) << "GLBackend::syncGPUObject(const Texture&) case for Texture Type " << texture.getType() << " not supported";    
    }
    (void) CHECK_GL_ERROR();

    return object;
}



GLuint GLBackend::getTextureID(const TexturePointer& texture) {
    if (!texture) {
        return 0;
    }
    GLTexture* object = GLBackend::syncGPUObject(*texture);
    if (object) {
        return object->_texture;
    } else {
        return 0;
    }
}

void GLBackend::syncSampler(const Sampler& sampler, Texture::Type type, GLTexture* object) {
    if (!object) return;
    if (!object->_texture) return;

    class GLFilterMode {
    public:
        GLint minFilter;
        GLint magFilter;
    };
    static const GLFilterMode filterModes[] = {   
        {GL_NEAREST,      GL_NEAREST},  //FILTER_MIN_MAG_POINT,
        {GL_NEAREST,       GL_LINEAR},  //FILTER_MIN_POINT_MAG_LINEAR,
        {GL_LINEAR,      GL_NEAREST},  //FILTER_MIN_LINEAR_MAG_POINT,
        {GL_LINEAR,       GL_LINEAR},  //FILTER_MIN_MAG_LINEAR,
 
        {GL_NEAREST_MIPMAP_NEAREST,      GL_NEAREST},  //FILTER_MIN_MAG_MIP_POINT,
        {GL_NEAREST_MIPMAP_NEAREST,      GL_NEAREST},  //FILTER_MIN_MAG_MIP_POINT,
        {GL_NEAREST_MIPMAP_LINEAR,       GL_NEAREST},  //FILTER_MIN_MAG_POINT_MIP_LINEAR,
        {GL_NEAREST_MIPMAP_NEAREST,      GL_LINEAR},  //FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        {GL_NEAREST_MIPMAP_LINEAR,       GL_LINEAR},  //FILTER_MIN_POINT_MAG_MIP_LINEAR,
        {GL_LINEAR_MIPMAP_NEAREST,       GL_NEAREST},  //FILTER_MIN_LINEAR_MAG_MIP_POINT,
        {GL_LINEAR_MIPMAP_LINEAR,        GL_NEAREST},  //FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        {GL_LINEAR_MIPMAP_NEAREST,       GL_LINEAR},  //FILTER_MIN_MAG_LINEAR_MIP_POINT,
        {GL_LINEAR_MIPMAP_LINEAR,        GL_LINEAR},  //FILTER_MIN_MAG_MIP_LINEAR,
        {GL_LINEAR_MIPMAP_LINEAR,        GL_LINEAR}  //FILTER_ANISOTROPIC,
    };

    auto fm = filterModes[sampler.getFilter()];
    glTexParameteri(object->_target, GL_TEXTURE_MIN_FILTER, fm.minFilter);
    glTexParameteri(object->_target, GL_TEXTURE_MAG_FILTER, fm.magFilter);

    static const GLenum comparisonFuncs[] = { 
        GL_NEVER,
        GL_LESS,
        GL_EQUAL,
        GL_LEQUAL,
        GL_GREATER,
        GL_NOTEQUAL,
        GL_GEQUAL,
        GL_ALWAYS };

    if (sampler.doComparison()) {
        glTexParameteri(object->_target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
        glTexParameteri(object->_target, GL_TEXTURE_COMPARE_FUNC, comparisonFuncs[sampler.getComparisonFunction()]);
    } else {
        glTexParameteri(object->_target, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    }

    static const GLenum wrapModes[] = {   
        GL_REPEAT,                         // WRAP_REPEAT,
        GL_MIRRORED_REPEAT,                // WRAP_MIRROR,
        GL_CLAMP_TO_EDGE,                  // WRAP_CLAMP,
        GL_CLAMP_TO_BORDER,                // WRAP_BORDER,
        GL_MIRROR_CLAMP_TO_EDGE_EXT };     // WRAP_MIRROR_ONCE,

    glTexParameteri(object->_target, GL_TEXTURE_WRAP_S, wrapModes[sampler.getWrapModeU()]);
    glTexParameteri(object->_target, GL_TEXTURE_WRAP_T, wrapModes[sampler.getWrapModeV()]);
    glTexParameteri(object->_target, GL_TEXTURE_WRAP_R, wrapModes[sampler.getWrapModeW()]);

    glTexParameterfv(object->_target, GL_TEXTURE_BORDER_COLOR, (const float*) &sampler.getBorderColor());
    glTexParameteri(object->_target, GL_TEXTURE_BASE_LEVEL, sampler.getMipOffset());
    glTexParameterf(object->_target, GL_TEXTURE_MIN_LOD, (float) sampler.getMinMip());
    glTexParameterf(object->_target, GL_TEXTURE_MAX_LOD, (sampler.getMaxMip() == Sampler::MAX_MIP_LEVEL ? 1000.f : sampler.getMaxMip()));
    glTexParameterf(object->_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, sampler.getMaxAnisotropy());

}
