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
#include "GLTexelFormat.h"

using namespace gpu;

GLBackend::GLTexture::GLTexture() :
    _storageStamp(0),
    _contentStamp(0),
    _texture(0),
    _target(GL_TEXTURE_2D),
    _size(0)
{
    Backend::incrementTextureGPUCount();
}

GLBackend::GLTexture::~GLTexture() {
    if (_texture != 0) {
        glDeleteTextures(1, &_texture);
    }
    Backend::updateTextureGPUMemoryUsage(_size, 0);
    Backend::decrementTextureGPUCount();
}

void GLBackend::GLTexture::setSize(GLuint size) {
    Backend::updateTextureGPUMemoryUsage(_size, size);
    _size = size;
}

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
                    const GLvoid* bytes = mip->readData();
                    Element srcFormat = mip->getFormat();
                
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
                
                    bytes = mip->readData();
                    srcFormat = mip->getFormat();

                    object->_contentStamp = texture.getDataStamp();
                }

                GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);

                glTexImage2D(GL_TEXTURE_2D, 0,
                    texelFormat.internalFormat, texture.getWidth(), texture.getHeight(), 0,
                    texelFormat.format, texelFormat.type, bytes);

                if (bytes && texture.isAutogenerateMips()) {
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                }
                object->_target = GL_TEXTURE_2D;

                syncSampler(texture.getSampler(), texture.getType(), object);
                
                // At this point the mip pixels have been loaded, we can notify
                texture.notifyMipFaceGPULoaded(0, 0);

                object->_storageStamp = texture.getStamp();
                object->_contentStamp = texture.getDataStamp();
                object->setSize((GLuint)texture.getSize());
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
                        Element srcFormat = mipFace->getFormat();
                        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);

                        glTexSubImage2D(FACE_LAYOUT[f], 0, texelFormat.internalFormat, texture.getWidth(), texture.getWidth(), 0,
                                texelFormat.format, texelFormat.type, (GLvoid*) (mipFace->readData()));

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
                        Element srcFormat = mipFace->getFormat();
                        GLTexelFormat texelFormat = GLTexelFormat::evalGLTexelFormat(texture.getTexelFormat(), srcFormat);

                        glTexImage2D(FACE_LAYOUT[f], 0, texelFormat.internalFormat, texture.getWidth(), texture.getWidth(), 0,
                                texelFormat.format, texelFormat.type, (GLvoid*) (mipFace->readData()));

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
                object->setSize((GLuint)texture.getSize());
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



GLuint GLBackend::getTextureID(const TexturePointer& texture, bool sync) {
    if (!texture) {
        return 0;
    }
    GLTexture* object { nullptr };
    if (sync) {
        object = GLBackend::syncGPUObject(*texture);
    } else {
        object = Backend::getGPUObject<GLBackend::GLTexture>(*texture);
    }
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



void GLBackend::do_generateTextureMips(Batch& batch, size_t paramOffset) {
    TexturePointer resourceTexture = batch._textures.get(batch._params[paramOffset + 0]._uint);
    if (!resourceTexture) {
        return;
    }

    GLTexture* object = GLBackend::syncGPUObject(*resourceTexture);
    if (!object) {
        return;
    }

    // IN 4.1 we still need to find an available slot
    auto freeSlot = _resource.findEmptyTextureSlot();
    auto bindingSlot = (freeSlot < 0 ? 0 : freeSlot);
    glActiveTexture(GL_TEXTURE0 + bindingSlot);
    glBindTexture(object->_target, object->_texture);

    glGenerateMipmap(object->_target);

    if (freeSlot < 0) {
        // If had to use slot 0 then restore state
        GLTexture* boundObject = GLBackend::syncGPUObject(*_resource._textures[0]);
        if (boundObject) {
            glBindTexture(boundObject->_target, boundObject->_texture);
        }
    } else {
        // clean up
        glBindTexture(object->_target, 0);
    }
    (void)CHECK_GL_ERROR();
}
