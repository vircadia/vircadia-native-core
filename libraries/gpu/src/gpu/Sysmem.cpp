//
//  Created by Sam Gateau on 10/8/2014.
//  Split from Resource.h/Resource.cpp by Bradley Austin Davis on 2016/08/07
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Sysmem.h"

#include <QtCore/QDebug>

using namespace gpu;

Size Sysmem::allocateMemory(Byte** dataAllocated, Size size) {
    if ( !dataAllocated ) { 
        qWarning() << "Buffer::Sysmem::allocateMemory() : Must have a valid dataAllocated pointer.";
        return NOT_ALLOCATED;
    }

    // Try to allocate if needed
    Size newSize = 0;
    if (size > 0) {
        // Try allocating as much as the required size + one block of memory
        newSize = size;
        (*dataAllocated) = new (std::nothrow) Byte[newSize];
        // Failed?
        if (!(*dataAllocated)) {
            qWarning() << "Buffer::Sysmem::allocate() : Can't allocate a system memory buffer of " << newSize << "bytes. Fails to create the buffer Sysmem.";
            return NOT_ALLOCATED;
        }
    }

    // Return what's actually allocated
    return newSize;
}

void Sysmem::deallocateMemory(Byte* dataAllocated, Size size) {
    if (dataAllocated) {
        delete[] dataAllocated;
    }
}

Sysmem::Sysmem() {}

Sysmem::Sysmem(Size size, const Byte* bytes) {
    if (size > 0 && bytes) {
        setData(_size, bytes);
    }
}

Sysmem::Sysmem(const Sysmem& sysmem) {
    if (sysmem.getSize() > 0) {
        allocate(sysmem._size);
        setData(_size, sysmem._data);
    }
}

Sysmem& Sysmem::operator=(const Sysmem& sysmem) {
    setData(sysmem.getSize(), sysmem.readData());
    return (*this);
}

Sysmem::~Sysmem() {
    deallocateMemory( _data, _size );
    _data = NULL;
    _size = 0;
}

Size Sysmem::allocate(Size size) {
    if (size != _size) {
        Byte* newData = NULL;
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

Size Sysmem::resize(Size size) {
    if (size != _size) {
        Byte* newData = NULL;
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

Size Sysmem::setData( Size size, const Byte* bytes ) {
    if (allocate(size) == size) {
        if (size && bytes) {
            memcpy( _data, bytes, _size );
        }
    }
    return _size;
}

Size Sysmem::setSubData( Size offset, Size size, const Byte* bytes) {
    if (size && ((offset + size) <= getSize()) && bytes) {
        memcpy( _data + offset, bytes, size );
        return size;
    }
    return 0;
}

Size Sysmem::append(Size size, const Byte* bytes) {
    if (size > 0) {
        Size oldSize = getSize();
        Size totalSize = oldSize + size;
        if (resize(totalSize) == totalSize) {
            return setSubData(oldSize, size, bytes);
        }
    }
    return 0;
}

