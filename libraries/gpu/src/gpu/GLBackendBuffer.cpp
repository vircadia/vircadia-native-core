//
//  GLBackendBuffer.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackendShared.h"

using namespace gpu;

GLuint allocateSingleBuffer() {
    GLuint result;
    glGenBuffers(1, &result);
    (void)CHECK_GL_ERROR();
    return result;
}

GLBackend::GLBuffer::GLBuffer(const Buffer& buffer, GLBuffer* original) :
    _buffer(allocateSingleBuffer()),
    _size((GLuint)buffer._sysmem.getSize()),
    _stamp(buffer._sysmem.getStamp()),
    _gpuBuffer(buffer) {
    glBindBuffer(GL_ARRAY_BUFFER, _buffer);
    if (GLEW_VERSION_4_4 || GLEW_ARB_buffer_storage) {
        glBufferStorage(GL_ARRAY_BUFFER, _size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        (void)CHECK_GL_ERROR();
    } else {
        glBufferData(GL_ARRAY_BUFFER, _size, nullptr, GL_DYNAMIC_DRAW);
        (void)CHECK_GL_ERROR();
    }
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
    Backend::incrementBufferGPUCount();
    Backend::updateBufferGPUMemoryUsage(0, _size);
}

GLBackend::GLBuffer::~GLBuffer() {
    if (_buffer != 0) {
        glDeleteBuffers(1, &_buffer);
    }
    Backend::updateBufferGPUMemoryUsage(_size, 0);
    Backend::decrementBufferGPUCount();
}

void GLBackend::GLBuffer::transfer() {
    glBindBuffer(GL_ARRAY_BUFFER, _buffer);
    (void)CHECK_GL_ERROR();
    GLintptr offset;
    GLsizeiptr size;
    size_t currentPage { 0 };
    auto data = _gpuBuffer.getSysmem().readData();
    while (getNextTransferBlock(offset, size, currentPage)) {
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data + offset);
        (void)CHECK_GL_ERROR();
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    (void)CHECK_GL_ERROR();
    _gpuBuffer._flags &= ~Buffer::DIRTY;
}

bool GLBackend::GLBuffer::getNextTransferBlock(GLintptr& outOffset, GLsizeiptr& outSize, size_t& currentPage) const {
    size_t pageCount = _gpuBuffer._pages.size();
    // Advance to the first dirty page
    while (currentPage < pageCount && (0 == (Buffer::DIRTY & _gpuBuffer._pages[currentPage]))) {
        ++currentPage;
    }

    // If we got to the end, we're done
    if (currentPage >= pageCount) {
        return false;
    }

    // Advance to the next clean page
    outOffset = static_cast<GLintptr>(currentPage * _gpuBuffer._pageSize);
    while (currentPage < pageCount && (0 != (Buffer::DIRTY & _gpuBuffer._pages[currentPage]))) {
        _gpuBuffer._pages[currentPage] &= ~Buffer::DIRTY;
        ++currentPage;
    }
    outSize = static_cast<GLsizeiptr>((currentPage * _gpuBuffer._pageSize) - outOffset);
    return true;
}

GLBackend::GLBuffer* GLBackend::syncGPUObject(const Buffer& buffer) {
    GLBuffer* object = Backend::getGPUObject<GLBackend::GLBuffer>(buffer);

    // Has the storage size changed?
    if (!object || object->_stamp != buffer.getSysmem().getStamp()) {
        object = new GLBuffer(buffer, object);
    }

    if (0 != (buffer._flags & Buffer::DIRTY)) {
        object->transfer();
    }

    return object;
}



GLuint GLBackend::getBufferID(const Buffer& buffer) {
    GLBuffer* bo = GLBackend::syncGPUObject(buffer);
    if (bo) {
        return bo->_buffer;
    } else {
        return 0;
    }
}

