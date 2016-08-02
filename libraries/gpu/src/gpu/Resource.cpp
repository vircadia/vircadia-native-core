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

#include "Context.h"

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

Size Sysmem::allocateMemory(Byte** dataAllocated, Size size) {
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

void Sysmem::deallocateMemory(Byte* dataAllocated, Size size) {
    allocationDebugger -= size;
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

std::atomic<uint32_t> Buffer::_bufferCPUCount{ 0 };
std::atomic<Buffer::Size> Buffer::_bufferCPUMemoryUsage{ 0 };

void Buffer::updateBufferCPUMemoryUsage(Size prevObjectSize, Size newObjectSize) {
    if (prevObjectSize == newObjectSize) {
        return;
    }
    if (prevObjectSize > newObjectSize) {
        _bufferCPUMemoryUsage.fetch_sub(prevObjectSize - newObjectSize);
    } else {
        _bufferCPUMemoryUsage.fetch_add(newObjectSize - prevObjectSize);
    }
}

uint32_t Buffer::getBufferCPUCount() {
    return _bufferCPUCount.load();
}

Buffer::Size Buffer::getBufferCPUMemoryUsage() {
    return _bufferCPUMemoryUsage.load();
}

uint32_t Buffer::getBufferGPUCount() {
    return Context::getBufferGPUCount();
}

Buffer::Size Buffer::getBufferGPUMemoryUsage() {
    return Context::getBufferGPUMemoryUsage();
}

Buffer::Buffer(Size pageSize) :
    _pages(pageSize) {
    _bufferCPUCount++;
}

Buffer::Buffer(Size size, const Byte* bytes, Size pageSize) : Buffer(pageSize) {
    setData(size, bytes);
}

Buffer::Buffer(const Buffer& buf) : Buffer(buf._pages._pageSize) {
    setData(buf.getSize(), buf.getData());
}

Buffer& Buffer::operator=(const Buffer& buf) {
    const_cast<Size&>(_pages._pageSize) = buf._pages._pageSize;
    setData(buf.getSize(), buf.getData());
    return (*this);
}

Buffer::~Buffer() {
    _bufferCPUCount--;
    Buffer::updateBufferCPUMemoryUsage(_sysmem.getSize(), 0);
}

Buffer::Size Buffer::resize(Size size) {
    _end = size;
    auto prevSize = _sysmem.getSize();
    if (prevSize < size) {
        _sysmem.resize(_pages.accommodate(_end));
        Buffer::updateBufferCPUMemoryUsage(prevSize, _sysmem.getSize());
    }
    return _end;
}

void Buffer::markDirty(Size offset, Size bytes) {
    if (!bytes) {
        return;
    }

    _pages.markRegion(offset, bytes);
}

Buffer::Update Buffer::getUpdate() const {
    ++_getUpdateCount;
    static Update EMPTY_UPDATE;
    if (!_pages) {
        return EMPTY_UPDATE;
    }

    Update result;
    result.pages = _pages;
    Size bufferSize = _sysmem.getSize();
    Size pageSize = _pages._pageSize;
    PageManager::Pages dirtyPages = _pages.getMarkedPages();
    std::vector<uint8> dirtyPageData;
    dirtyPageData.resize(dirtyPages.size() * pageSize);
    for (Size i = 0; i < dirtyPages.size(); ++i) {
        Size page = dirtyPages[i];
        Size sourceOffset = page * pageSize;
        Size destOffset = i * pageSize;
        memcpy(dirtyPageData.data() + destOffset, _sysmem.readData() + sourceOffset, pageSize);
    }

    result.updateOperator = [bufferSize, pageSize, dirtyPages, dirtyPageData](Sysmem& dest){
        dest.resize(bufferSize);
        for (Size i = 0; i < dirtyPages.size(); ++i) {
            Size page = dirtyPages[i];
            Size sourceOffset = i * pageSize;
            Size destOffset = page * pageSize;
            memcpy(dest.editData() + destOffset, dirtyPageData.data() + sourceOffset, pageSize);
        }
    };
    return result;
}

void Buffer::applyUpdate(const Update& update) {
    ++_applyUpdateCount;
    _renderPages = update.pages;
    update.updateOperator(_renderSysmem);
}

void Buffer::flush() {
    ++_getUpdateCount;
    ++_applyUpdateCount;
    _renderPages = _pages;
    _renderSysmem.resize(_sysmem.getSize());
    auto dirtyPages = _pages.getMarkedPages();
    for (Size page : dirtyPages) {
        Size offset = page * _pages._pageSize;
        memcpy(_renderSysmem.editData() + offset, _sysmem.readData() + offset, _pages._pageSize);
    }
}

Buffer::Size Buffer::setData(Size size, const Byte* data) {
    resize(size);
    setSubData(0, size, data);
    return _end;
}

Buffer::Size Buffer::setSubData(Size offset, Size size, const Byte* data) {
    auto changedBytes = editSysmem().setSubData(offset, size, data);
    if (changedBytes) {
        markDirty(offset, changedBytes);
    }
    return changedBytes;
}

Buffer::Size Buffer::append(Size size, const Byte* data) {
    auto offset = _end;
    resize(_end + size);
    setSubData(offset, size, data);
    return _end;
}

Buffer::Size Buffer::getSize() const { 
    Q_ASSERT(getSysmem().getSize() >= _end);
    return _end;
}

const Element BufferView::DEFAULT_ELEMENT = Element( gpu::SCALAR, gpu::UINT8, gpu::RAW );

BufferView::BufferView() :
BufferView(DEFAULT_ELEMENT) {}

BufferView::BufferView(const Element& element) :
    BufferView(BufferPointer(), element) {}

BufferView::BufferView(Buffer* newBuffer, const Element& element) :
    BufferView(BufferPointer(newBuffer), element) {}

BufferView::BufferView(const BufferPointer& buffer, const Element& element) :
    BufferView(buffer, DEFAULT_OFFSET, buffer ? buffer->getSize() : 0, element.getSize(), element) {}

BufferView::BufferView(const BufferPointer& buffer, Size offset, Size size, const Element& element) :
    BufferView(buffer, offset, size, element.getSize(), element) {}

BufferView::BufferView(const BufferPointer& buffer, Size offset, Size size, uint16 stride, const Element& element) :
    _buffer(buffer),
    _offset(offset),
    _size(size),
    _element(element),
    _stride(stride) {}
