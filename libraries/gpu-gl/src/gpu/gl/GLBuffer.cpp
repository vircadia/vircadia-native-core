//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GLBuffer.h"
#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;

GLBuffer::~GLBuffer() {
    if (_id) {
        auto backend = _backend.lock();
        if (backend) {
            backend->releaseBuffer(_id, _size);
        }
    }
}

GLBuffer::GLBuffer(const std::weak_ptr<GLBackend>& backend, const Buffer& buffer, GLuint id) :
    GLObject(backend, buffer, id),
    _size((GLuint)buffer._renderSysmem.getSize()),
    _stamp(buffer._renderSysmem.getStamp())
{
    Backend::incrementBufferGPUCount();
    Backend::updateBufferGPUMemoryUsage(0, _size);
}

