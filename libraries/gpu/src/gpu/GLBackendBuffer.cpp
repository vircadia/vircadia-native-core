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

static std::once_flag check_dsa;
static bool DSA_SUPPORTED { false };

GLuint allocateSingleBuffer() {
    std::call_once(check_dsa, [&] {
        DSA_SUPPORTED = (GLEW_VERSION_4_5 || GLEW_ARB_direct_state_access);
    });
    GLuint result;
    if (DSA_SUPPORTED) {
        glCreateBuffers(1, &result);
    } else {
        glGenBuffers(1, &result);
    }
    return result;
}

GLBackend::GLBuffer::GLBuffer(const Buffer& buffer) : 
    _buffer(allocateSingleBuffer()), 
    _size(buffer._sysmem.getSize()), 
    _stamp(buffer._sysmem.getStamp()), 
    _gpuBuffer(buffer) {
    (void)CHECK_GL_ERROR();
    Backend::setGPUObject(buffer, this);
    if (DSA_SUPPORTED) {
        glNamedBufferStorage(_buffer, _size, nullptr, GL_DYNAMIC_STORAGE_BIT);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, _buffer);
        if (GLEW_VERSION_4_4 || GLEW_ARB_buffer_storage) {
            glBufferStorage(GL_ARRAY_BUFFER, _size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        } else {
            glBufferData(GL_ARRAY_BUFFER, buffer.getSysmem().getSize(), buffer.getSysmem().readData(), GL_DYNAMIC_DRAW);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    Backend::incrementBufferGPUCount();
}
    
GLBackend::GLBuffer::~GLBuffer() {
    if (_buffer != 0) {
        glDeleteBuffers(1, &_buffer);
    }
    Backend::updateBufferGPUMemoryUsage(_size, 0);
    Backend::decrementBufferGPUCount();
}

void GLBackend::GLBuffer::transfer(bool forceAll) {
    const auto& pageFlags = _gpuBuffer._pages;
    if (!forceAll) {
        size_t transitions = 0;
        if (pageFlags.size()) {
            bool lastDirty = (0 != (pageFlags[0] & Buffer::DIRTY));
            for (size_t i = 1; i < pageFlags.size(); ++i) {
                bool newDirty = (0 != (pageFlags[0] & Buffer::DIRTY));
                if (newDirty != lastDirty) {
                    ++transitions;
                    lastDirty = newDirty;
                }
            }
        }

        // If there are no transitions (implying the whole buffer is dirty) 
        // or more than 20 transitions, then just transfer the whole buffer
        if (transitions == 0 || transitions > 20) {
            forceAll = true;
        }
    }

    // Are we transferring the whole buffer?
    if (forceAll) {
        if (DSA_SUPPORTED) {
            glNamedBufferSubData(_buffer, 0, _size, _gpuBuffer.getSysmem().readData());
        } else {
            // Now let's update the content of the bo with the sysmem version
            // TODO: in the future, be smarter about when to actually upload the glBO version based on the data that did change
            //if () {
            glBindBuffer(GL_ARRAY_BUFFER, _buffer);
            glBufferData(GL_ARRAY_BUFFER, _gpuBuffer.getSysmem().getSize(), _gpuBuffer.getSysmem().readData(), GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    } else {
        if (!DSA_SUPPORTED) {
            glBindBuffer(GL_ARRAY_BUFFER, _buffer);
        }
        GLintptr offset;
        GLsizeiptr size;
        size_t currentPage { 0 };
        auto data = _gpuBuffer.getSysmem().readData();
        while (getNextTransferBlock(offset, size, currentPage)) {
            if (DSA_SUPPORTED) {
                glNamedBufferSubData(_buffer, offset, size, data + offset);
            } else {
                glBufferSubData(GL_ARRAY_BUFFER, offset, size, data + offset);
            }
        }

        if (!DSA_SUPPORTED) {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
    _gpuBuffer._flags &= ~Buffer::DIRTY;
    (void)CHECK_GL_ERROR();
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
        ++currentPage;
    }
    outSize = static_cast<GLsizeiptr>((currentPage * _gpuBuffer._pageSize) - outOffset);
    return true;
}

GLBackend::GLBuffer* GLBackend::syncGPUObject(const Buffer& buffer) {
    GLBuffer* object = Backend::getGPUObject<GLBackend::GLBuffer>(buffer);

    bool forceTransferAll = false;
    // Has the storage size changed?
    if (!object || object->_stamp != buffer.getSysmem().getStamp()) {
        object = new GLBuffer(buffer);
        forceTransferAll = true;
    }

    if (forceTransferAll || (0 != (buffer._flags & Buffer::DIRTY))) {
        object->transfer(forceTransferAll);
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

