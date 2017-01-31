//
//  Created by Sam Gateau on 10/8/2014.
//  Split from Resource.h/Resource.cpp by Bradley Austin Davis on 2016/08/07
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Buffer.h"
#include "Context.h"

using namespace gpu;

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

void Buffer::incrementBufferCPUCount() {
    _bufferCPUCount++;
}

void Buffer::decrementBufferCPUCount() {
    _bufferCPUCount--;
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
    _renderPages(pageSize), _pages(pageSize) {
    Buffer::incrementBufferCPUCount();
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
    Buffer::decrementBufferCPUCount();
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

extern bool isRenderThread();

Buffer::Update::Update(const Update& other) : 
    buffer(other.buffer),
    updateNumber(other.updateNumber),
    size(other.size),
    dirtyPages(other.dirtyPages),
    dirtyData(other.dirtyData) { }

Buffer::Update::Update(Update&& other) : 
    buffer(other.buffer),
    updateNumber(other.updateNumber),
    size(other.size),
    dirtyPages(std::move(other.dirtyPages)),
    dirtyData(std::move(other.dirtyData)) { }

Buffer::Update::Update(const Buffer& parent) : buffer(parent) {
    const auto pageSize = buffer._pages._pageSize;
    updateNumber = ++buffer._getUpdateCount;
    size = buffer._sysmem.getSize();
    dirtyPages = buffer._pages.getMarkedPages();
    dirtyData.resize(dirtyPages.size() * pageSize, 0);
    for (Size i = 0; i < dirtyPages.size(); ++i) {
        Size page = dirtyPages[i];
        Size sourceOffset = page * pageSize;
        Size destOffset = i * pageSize;
        assert(dirtyData.size() >= (destOffset + pageSize));
        assert(buffer._sysmem.getSize() >= (sourceOffset + pageSize));
        memcpy(dirtyData.data() + destOffset, buffer._sysmem.readData() + sourceOffset, pageSize);
    }
}

void Buffer::Update::apply() const {
    // Make sure we're loaded in order
    ++buffer._applyUpdateCount;
    assert(buffer._applyUpdateCount.load() == updateNumber);
    const auto pageSize = buffer._pages._pageSize;
    buffer._renderSysmem.resize(size);
    buffer._renderPages.accommodate(size);
    for (Size i = 0; i < dirtyPages.size(); ++i) {
        Size page = dirtyPages[i];
        Size sourceOffset = i * pageSize;
        assert(dirtyData.size() >= (sourceOffset + pageSize));
        Size destOffset = page * pageSize;
        assert(buffer._renderSysmem.getSize() >= (destOffset + pageSize));
        memcpy(buffer._renderSysmem.editData() + destOffset, dirtyData.data() + sourceOffset, pageSize);
        buffer._renderPages.markPage(page);
    }
}

Buffer::Update Buffer::getUpdate() const {
    return Update(*this);
}

void Buffer::applyUpdate(const Update& update) {
    update.apply();
}

void Buffer::flush() const {
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
