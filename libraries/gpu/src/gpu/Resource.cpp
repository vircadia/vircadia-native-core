//
//  Resource.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/8/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Resource.h"

#include <atomic>

#include <SharedUtil.h>
#include <NumericalConstants.h>
#include <QDebug>

using namespace gpu;

class AllocationDebugger {
public:
    void operator+=(size_t size) {
        _allocatedMemory += size;
        maybeReport();
    }

    void operator-=(size_t size) {
        _allocatedMemory -= size;
        maybeReport();
    }

private:
    QString formatSize(size_t size) {
        float num = size;
        QStringList list;
        list << "KB" << "MB" << "GB" << "TB";

        QStringListIterator i(list);
        QString unit("bytes");

        while (num >= K && i.hasNext()) {
            unit = i.next();
            num /= K;
        }
        return QString().setNum(num, 'f', 2) + " " + unit;
    }

    void maybeReport() {
        auto now = usecTimestampNow();
        if (now - _lastReportTime < MAX_REPORT_FREQUENCY) {
            return;
        }
        size_t current = _allocatedMemory;
        size_t last = _lastReportedMemory;
        size_t delta = (current > last) ? (current - last) : (last - current);
        if (delta > MIN_REPORT_DELTA) {
            _lastReportTime = now;
            _lastReportedMemory = current;
            qDebug() << "Total allocation " << formatSize(current);
        }
    }

    std::atomic<size_t> _allocatedMemory;
    std::atomic<size_t> _lastReportedMemory;
    std::atomic<uint64_t> _lastReportTime;

    static const float K;
    // Report changes of 5 megabytes
    static const size_t MIN_REPORT_DELTA = 1024 * 1024 * 5;
    // Report changes no more frequently than every 15 seconds
    static const uint64_t MAX_REPORT_FREQUENCY = USECS_PER_SECOND * 15;
};

const float AllocationDebugger::K = 1024.0f;

static AllocationDebugger allocationDebugger;

Resource::Size Resource::Sysmem::allocateMemory(Byte** dataAllocated, Size size) {
    allocationDebugger += size;
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

void Resource::Sysmem::deallocateMemory(Byte* dataAllocated, Size size) {
    allocationDebugger -= size;
    if (dataAllocated) {
        delete[] dataAllocated;
    }
}

Resource::Sysmem::Sysmem() :
    _stamp(0),
    _size(0),
    _data(NULL)
{
}

Resource::Sysmem::Sysmem(Size size, const Byte* bytes) :
    _stamp(0),
    _size(0),
    _data(NULL)
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

Resource::Sysmem::Sysmem(const Sysmem& sysmem) :
    _stamp(0),
    _size(0),
    _data(NULL)
{
    if (sysmem.getSize() > 0) {
        _size = allocateMemory(&_data, sysmem.getSize());
        if (_size >= sysmem.getSize()) {
            if (sysmem.readData()) {
                memcpy(_data, sysmem.readData(), sysmem.getSize());
            }
        }
    }
}

Resource::Sysmem& Resource::Sysmem::operator=(const Sysmem& sysmem) {
    setData(sysmem.getSize(), sysmem.readData());
    return (*this);
}

Resource::Sysmem::~Sysmem() {
    deallocateMemory( _data, _size );
    _data = NULL;
    _size = 0;
}

Resource::Size Resource::Sysmem::allocate(Size size) {
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

Resource::Size Resource::Sysmem::resize(Size size) {
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

Resource::Size Resource::Sysmem::setData( Size size, const Byte* bytes ) {
    if (allocate(size) == size) {
        if (size && bytes) {
            memcpy( _data, bytes, _size );
            _stamp++;
        }
    }
    return _size;
}

Resource::Size Resource::Sysmem::setSubData( Size offset, Size size, const Byte* bytes) {
    if (size && ((offset + size) <= getSize()) && bytes) {
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
    _sysmem(new Sysmem()) {
}

Buffer::Buffer(Size size, const Byte* bytes) :
    Resource(),
    _sysmem(new Sysmem(size, bytes)) {
}

Buffer::Buffer(const Buffer& buf) :
    Resource(),
    _sysmem(new Sysmem(buf.getSysmem())) {
}

Buffer& Buffer::operator=(const Buffer& buf) {
    (*_sysmem) = buf.getSysmem();
    return (*this);
}

Buffer::~Buffer() {
    if (_sysmem) {
        delete _sysmem;
        _sysmem = NULL;
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

