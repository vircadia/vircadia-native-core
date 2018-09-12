//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"
#include <gpu/gl/GLBuffer.h>

namespace gpu {
    namespace gl41 {
        class GL41Buffer : public gpu::gl::GLBuffer {
            using Parent = gpu::gl::GLBuffer;
            static GLuint allocate() {
                GLuint result;
                glGenBuffers(1, &result);
                return result;
            }

            ~GL41Buffer() { 
                if (_texBuffer) {
                    glDeleteTextures(1, &_texBuffer);
                }
            }

        public:
            GL41Buffer(const std::weak_ptr<gl::GLBackend>& backend, const Buffer& buffer, GL41Buffer* original) : Parent(backend, buffer, allocate()) {
                glBindBuffer(GL_COPY_WRITE_BUFFER, _buffer);
                glBufferData(GL_COPY_WRITE_BUFFER, _size, nullptr, GL_DYNAMIC_DRAW);
                glBindBuffer(GL_COPY_WRITE_BUFFER, 0);

                if (original && original->_size) {
                    glBindBuffer(GL_COPY_WRITE_BUFFER, _buffer);
                    glBindBuffer(GL_COPY_READ_BUFFER, original->_buffer);
                    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, original->_size);
                    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                    glBindBuffer(GL_COPY_READ_BUFFER, 0);
                    (void)CHECK_GL_ERROR();
                }
                Backend::setGPUObject(buffer, this);
            }

            void transfer() override {
                glBindBuffer(GL_COPY_WRITE_BUFFER, _buffer);
                (void)CHECK_GL_ERROR();
                Size offset;
                Size size;
                Size currentPage { 0 };
                auto data = _gpuObject._renderSysmem.readData();
                while (_gpuObject._renderPages.getNextTransferBlock(offset, size, currentPage)) {
                    glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data + offset);
                    (void)CHECK_GL_ERROR();
                }
                glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
                (void)CHECK_GL_ERROR();
                _gpuObject._renderPages._flags &= ~PageManager::DIRTY;
            }

            // REsource BUffer are implemented with TextureBuffer 
            GLuint _texBuffer { 0 };
            GLuint getTexBufferId() {
                if (!_texBuffer) {
                    glGenTextures(1, &_texBuffer);
                    glActiveTexture(GL_TEXTURE0 + GL41Backend::RESOURCE_BUFFER_TEXBUF_TEX_UNIT);
                    glBindTexture(GL_TEXTURE_BUFFER, _texBuffer);
                    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _buffer);
                    glBindTexture(GL_TEXTURE_BUFFER, 0);
                    (void)CHECK_GL_ERROR();
                }
                return _texBuffer;
            }
        };
    }
}

using namespace gpu;
using namespace gpu::gl;
using namespace gpu::gl41;


GLuint GL41Backend::getBufferID(const Buffer& buffer) {
    return GL41Buffer::getId<GL41Buffer>(*this, buffer);
}

GLuint GL41Backend::getBufferIDUnsynced(const Buffer& buffer) {
    return GL41Buffer::getIdUnsynced<GL41Buffer>(*this, buffer);
}

GLuint GL41Backend::getResourceBufferID(const Buffer& buffer) {
    auto* object = GL41Buffer::sync<GL41Buffer>(*this, buffer);
    if (object) {
        return object->getTexBufferId();
    } else {
        return 0;
    }
}

GLBuffer* GL41Backend::syncGPUObject(const Buffer& buffer) {
    return GL41Buffer::sync<GL41Buffer>(*this, buffer);
}

bool GL41Backend::bindResourceBuffer(uint32_t slot, const BufferPointer& buffer) {
    GLuint texBuffer = GL41Backend::getResourceBufferID((*buffer));
    if (texBuffer) {
        glActiveTexture(GL_TEXTURE0 + GL41Backend::RESOURCE_BUFFER_SLOT0_TEX_UNIT + slot); 
        glBindTexture(GL_TEXTURE_BUFFER, texBuffer);

        (void)CHECK_GL_ERROR();

        assign(_resource._buffers[slot], buffer);

        return true;
    }

    return false;
}

void GL41Backend::releaseResourceBuffer(uint32_t slot) {
    auto& bufferReference = _resource._buffers[slot];
    auto buffer = acquire(bufferReference);
    if (buffer) {
        glActiveTexture(GL_TEXTURE0 + GL41Backend::RESOURCE_BUFFER_SLOT0_TEX_UNIT + slot); 
        glBindTexture(GL_TEXTURE_BUFFER, 0);
        reset(bufferReference);
    }
}
