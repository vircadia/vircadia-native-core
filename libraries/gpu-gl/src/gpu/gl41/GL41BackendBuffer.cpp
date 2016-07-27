//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL41Backend.h"
#include "../gl/GLBuffer.h"

using namespace gpu;
using namespace gpu::gl41;

class GL41Buffer : public gl::GLBuffer {
    using Parent = gpu::gl::GLBuffer;
    static GLuint allocate() {
        GLuint result;
        glGenBuffers(1, &result);
        return result;
    }

public:
    GL41Buffer(const Buffer& buffer, GL41Buffer* original) : Parent(buffer, allocate()) {
        glBindBuffer(GL_ARRAY_BUFFER, _buffer);
        glBufferData(GL_ARRAY_BUFFER, _size, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

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
        glBindBuffer(GL_ARRAY_BUFFER, _buffer);
        (void)CHECK_GL_ERROR();
        Size offset;
        Size size;
        Size currentPage { 0 };
        auto data = _gpuObject._renderSysmem.readData();
        while (_gpuObject._renderPages.getNextTransferBlock(offset, size, currentPage)) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, size, data + offset);
            (void)CHECK_GL_ERROR();
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        (void)CHECK_GL_ERROR();
        _gpuObject._renderPages._flags &= ~PageManager::DIRTY;
    }
};

GLuint GL41Backend::getBufferID(const Buffer& buffer) {
    return GL41Buffer::getId<GL41Buffer>(buffer);
}

gl::GLBuffer* GL41Backend::syncGPUObject(const Buffer& buffer) {
    return GL41Buffer::sync<GL41Buffer>(buffer);
}
