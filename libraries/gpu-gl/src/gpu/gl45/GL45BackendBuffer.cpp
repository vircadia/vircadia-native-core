//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GL45Backend.h"
#include "../gl/GLBuffer.h"

using namespace gpu;
using namespace gpu::gl45;

class GL45Buffer : public gl::GLBuffer {
    using Parent = gpu::gl::GLBuffer;
    static GLuint allocate() {
        GLuint result;
        glCreateBuffers(1, &result);
        return result;
    }

public:
    GL45Buffer(const Buffer& buffer, GLBuffer* original) : Parent(buffer, allocate()) {
        glNamedBufferStorage(_buffer, _size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        if (original && original->_size) {
            glCopyNamedBufferSubData(original->_buffer, _buffer, 0, 0, std::min(original->_size, _size));
        }
        Backend::setGPUObject(buffer, this);
    }

    void transfer() override {
        Size offset;
        Size size;
        Size currentPage { 0 };
        auto data = _gpuObject._renderSysmem.readData();
        while (_gpuObject._renderPages.getNextTransferBlock(offset, size, currentPage)) {
            glNamedBufferSubData(_buffer, (GLintptr)offset, (GLsizeiptr)size, data + offset);
        }
        (void)CHECK_GL_ERROR();
        _gpuObject._renderPages._flags &= ~PageManager::DIRTY;
    }
};

GLuint GL45Backend::getBufferID(const Buffer& buffer) {
    return GL45Buffer::getId<GL45Buffer>(buffer);
}

gl::GLBuffer* GL45Backend::syncGPUObject(const Buffer& buffer) {
    return GL45Buffer::sync<GL45Buffer>(buffer);
}
