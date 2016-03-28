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

GLBackend::GLBuffer::GLBuffer() :
    _stamp(0),
    _buffer(0),
    _size(0)
{
    Backend::incrementBufferGPUCount();
}

GLBackend::GLBuffer::~GLBuffer() {
    if (_buffer != 0) {
        glDeleteBuffers(1, &_buffer);
    }
    Backend::updateBufferGPUMemoryUsage(_size, 0);
    Backend::decrementBufferGPUCount();
}

void GLBackend::GLBuffer::setSize(GLuint size) {
    Backend::updateBufferGPUMemoryUsage(_size, size);
    _size = size;
}

GLBackend::GLBuffer* GLBackend::syncGPUObject(const Buffer& buffer) {
    GLBuffer* object = Backend::getGPUObject<GLBackend::GLBuffer>(buffer);

    if (object && (object->_stamp == buffer.getSysmem().getStamp())) {
        return object;
    }

    // need to have a gpu object?
    if (!object) {
        object = new GLBuffer();
        glGenBuffers(1, &object->_buffer);
        (void) CHECK_GL_ERROR();
        Backend::setGPUObject(buffer, object);
    }

    // Now let's update the content of the bo with the sysmem version
    // TODO: in the future, be smarter about when to actually upload the glBO version based on the data that did change
    //if () {
    glBindBuffer(GL_ARRAY_BUFFER, object->_buffer);
    glBufferData(GL_ARRAY_BUFFER, buffer.getSysmem().getSize(), buffer.getSysmem().readData(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    object->_stamp = buffer.getSysmem().getStamp();
    object->setSize((GLuint)buffer.getSysmem().getSize());
    //}
    (void) CHECK_GL_ERROR();

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

