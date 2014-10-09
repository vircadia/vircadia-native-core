//
//  Resource.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/8/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Resource.h"

#include <QDebug>

using namespace gpu;

Resource::Size Resource::Sysmem::allocateMemory(Byte** dataAllocated, Size size) {
    if ( !dataAllocated ) { 
        qWarning() << "Buffer::Sysmem::allocateMemory() : Must have a valid dataAllocated pointer.";
        return NOT_ALLOCATED;
    }

    // Try to allocate if needed
    Size newSize = 0;
    if (size > 0) {
        // Try allocating as much as the required size + one block of memory
        newSize = size;
        (*dataAllocated) = new Byte[newSize];
        // Failed?
        if (!(*dataAllocated)) {
            qWarning() << "Buffer::Sysmem::allocate() : Can't allocate a system memory buffer of " << newSize << "bytes. Fails to create the buffer Sysmem.";
            return NOT_ALLOCATED;
        }
    }

    // Return what's actually allocated
    return newSize;
}

void Resource::Sysmem::deallocateMemory(Byte* dataAllocated, Size size) {
    if (dataAllocated) {
        delete[] dataAllocated;
    }
}

Resource::Sysmem::Sysmem() :
    _data(NULL),
    _size(0),
    _stamp(0)
{
}

Resource::Sysmem::Sysmem(Size size, const Byte* bytes) :
    _data(NULL),
    _size(0),
    _stamp(0)
{
    if (size > 0) {
        _size = allocateMemory(&_data, size);
        if (_size >= size) {
            if (bytes) {
                memcpy(_data, bytes, size);
            }
        }
    }
}

Resource::Sysmem::~Sysmem() {
    deallocateMemory( _data, _size );
    _data = NULL;
    _size = 0;
}

Resource::Size Resource::Sysmem::allocate(Size size) {
    if (size != _size) {
        Byte* newData = 0;
        Size newSize = 0;
        if (size > 0) {
            Size allocated = allocateMemory(&newData, size);
            if (allocated == NOT_ALLOCATED) {
                // early exit because allocation failed
                return 0;
            }
            newSize = allocated;
        }
        // Allocation was successful, can delete previous data
        deallocateMemory(_data, _size);
        _data = newData;
        _size = newSize;
        _stamp++;
    }
    return _size;
}

Resource::Size Resource::Sysmem::resize(Size size) {
    if (size != _size) {
        Byte* newData = 0;
        Size newSize = 0;
        if (size > 0) {
            Size allocated = allocateMemory(&newData, size);
            if (allocated == NOT_ALLOCATED) {
                // early exit because allocation failed
                return _size;
            }
            newSize = allocated;
            // Restore back data from old buffer in the new one
            if (_data) {
                Size copySize = ((newSize < _size)? newSize: _size);
                memcpy( newData, _data, copySize);
            }
        }
        // Reallocation was successful, can delete previous data
        deallocateMemory(_data, _size);
        _data = newData;
        _size = newSize;
        _stamp++;
    }
    return _size;
}

Resource::Size Resource::Sysmem::setData( Size size, const Byte* bytes ) {
    if (allocate(size) == size) {
        if (bytes) {
            memcpy( _data, bytes, _size );
            _stamp++;
        }
    }
    return _size;
}

Resource::Size Resource::Sysmem::setSubData( Size offset, Size size, const Byte* bytes) {
    if (((offset + size) <= getSize()) && bytes) {
        memcpy( _data + offset, bytes, size );
        _stamp++;
        return size;
    }
    return 0;
}

Resource::Size Resource::Sysmem::append(Size size, const Byte* bytes) {
    if (size > 0) {
        Size oldSize = getSize();
        Size totalSize = oldSize + size;
        if (resize(totalSize) == totalSize) {
            return setSubData(oldSize, size, bytes);
        }
    }
    return 0;
}

Buffer::Buffer() :
    Resource(),
    _sysmem(NULL),
    _gpuObject(NULL) {
    _sysmem = new Sysmem();
}

Buffer::~Buffer() {
    if (_sysmem) {
        delete _sysmem;
        _sysmem = 0;
    }
    if (_gpuObject) {
        delete _gpuObject;
        _gpuObject = 0;
    }
}

Buffer::Size Buffer::resize(Size size) {
    return editSysmem().resize(size);
}

Buffer::Size Buffer::setData(Size size, const Byte* data) {
    return editSysmem().setData(size, data);
}

Buffer::Size Buffer::setSubData(Size offset, Size size, const Byte* data) {
    return editSysmem().setSubData( offset, size, data);
}

Buffer::Size Buffer::append(Size size, const Byte* data) {
    return editSysmem().append( size, data);
}

namespace gpu {
namespace backend {

BufferObject::~BufferObject() {
    if (_buffer!=0) {
        glDeleteBuffers(1, &_buffer);
    }
}

void syncGPUObject(const Buffer& buffer) {
    BufferObject* object = buffer.getGPUObject();

    if (object && (object->_stamp == buffer.getSysmem().getStamp())) {
        return;
    }

    // need to have a gpu object?
    if (!object) {
        object = new BufferObject();
        glGenBuffers(1, &object->_buffer);
        buffer.setGPUObject(object);
    }

    // Now let's update the content of the bo with the sysmem version
    // TODO: in the future, be smarter about when to actually upload the glBO version based on the data that did change
    //if () {
        glBindBuffer(GL_ARRAY_BUFFER, object->_buffer);
        glBufferData(GL_ARRAY_BUFFER, buffer.getSysmem().getSize(), buffer.getSysmem().readData(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        object->_stamp = buffer.getSysmem().getStamp();
        object->_size = buffer.getSysmem().getSize();
    //}
}

};
};
