//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLBuffer.h"

using namespace gpu;
using namespace gpu::gl;

GLBuffer::~GLBuffer() {
    glDeleteBuffers(1, &_id);
    Backend::decrementBufferGPUCount();
    Backend::updateBufferGPUMemoryUsage(_size, 0);
}

GLBuffer::GLBuffer(const Buffer& buffer, GLuint id) :
    GLObject(buffer, id),
    _size((GLuint)buffer._sysmem.getSize()),
    _stamp(buffer._sysmem.getStamp())
{
    Backend::incrementBufferGPUCount();
    Backend::updateBufferGPUMemoryUsage(0, _size);
}

