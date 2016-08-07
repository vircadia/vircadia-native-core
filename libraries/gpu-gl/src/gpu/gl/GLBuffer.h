//
//  Created by Bradley Austin Davis on 2016/05/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_gl_GLBuffer_h
#define hifi_gpu_gl_GLBuffer_h

#include "GLShared.h"

namespace gpu { namespace gl {

class GLBuffer : public GLObject<Buffer> {
public:
    template <typename GLBufferType>
    static GLBufferType* sync(GLBackend& backend, const Buffer& buffer) {
        if (buffer.getSysmem().getSize() != 0) {
            if (buffer._getUpdateCount == 0) {
                qWarning() << "Unsynced buffer";
            }
            if (buffer._getUpdateCount < buffer._applyUpdateCount) {
                qWarning() << "Unsynced buffer " << buffer._getUpdateCount << " " << buffer._applyUpdateCount;
            }
        }
        GLBufferType* object = Backend::getGPUObject<GLBufferType>(buffer);

        // Has the storage size changed?
        if (!object || object->_stamp != buffer._renderSysmem.getStamp()) {
            object = new GLBufferType(backend.shared_from_this(), buffer, object);
        }

        if (0 != (buffer._renderPages._flags & PageManager::DIRTY)) {
            object->transfer();
        }

        return object;
    }

    template <typename GLBufferType>
    static GLuint getId(GLBackend& backend, const Buffer& buffer) {
        GLBuffer* bo = sync<GLBufferType>(backend, buffer);
        if (bo) {
            return bo->_buffer;
        } else {
            return 0;
        }
    }

    const GLuint& _buffer { _id };
    const GLuint _size;
    const Stamp _stamp;

    ~GLBuffer();

    virtual void transfer() = 0;

protected:
    GLBuffer(const std::weak_ptr<GLBackend>& backend, const Buffer& buffer, GLuint id);
};

} }

#endif
