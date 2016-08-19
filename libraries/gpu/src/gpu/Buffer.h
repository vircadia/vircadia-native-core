//
//  Created by Sam Gateau on 10/8/2014.
//  Split from Resource.h/Resource.cpp by Bradley Austin Davis on 2016/08/07
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Buffer_h
#define hifi_gpu_Buffer_h

#include <atomic>

#if _DEBUG
#include <QtCore/QDebug>
#endif

#include "Forward.h"
#include "Format.h"
#include "Resource.h"
#include "Sysmem.h"
#include "PageManager.h"

namespace gpu {

class Buffer : public Resource {
    static std::atomic<uint32_t> _bufferCPUCount;
    static std::atomic<Size> _bufferCPUMemoryUsage;
    static void updateBufferCPUMemoryUsage(Size prevObjectSize, Size newObjectSize);

public:
    using Flag = PageManager::Flag;

    class Update {
    public:
        Update(const Buffer& buffer);
        Update(const Update& other);
        Update(Update&& other);
        void apply() const;

    private:
        const Buffer& buffer;
        size_t updateNumber;
        Size size;
        PageManager::Pages dirtyPages;
        std::vector<uint8> dirtyData;
    };

    static uint32_t getBufferCPUCount();
    static Size getBufferCPUMemoryUsage();
    static uint32_t getBufferGPUCount();
    static Size getBufferGPUMemoryUsage();

    Buffer(Size pageSize = PageManager::DEFAULT_PAGE_SIZE);
    Buffer(Size size, const Byte* bytes, Size pageSize = PageManager::DEFAULT_PAGE_SIZE);
    Buffer(const Buffer& buf); // deep copy of the sysmem buffer
    Buffer& operator=(const Buffer& buf); // deep copy of the sysmem buffer
    ~Buffer();

    // The size in bytes of data stored in the buffer
    Size getSize() const;
    template <typename T>
    Size getTypedSize() const { return getSize() / sizeof(T); };

    const Byte* getData() const { return getSysmem().readData(); }
    
    // Resize the buffer
    // Keep previous data [0 to min(pSize, mSize)]
    Size resize(Size pSize);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    // \return the size of the buffer
    Size setData(Size size, const Byte* data);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    // \return the number of bytes copied
    Size setSubData(Size offset, Size size, const Byte* data);

    template <typename T>
    Size setSubData(Size index, const T& t) {
        Size offset = index * sizeof(T);
        Size size = sizeof(T);
        return setSubData(offset, size, reinterpret_cast<const Byte*>(&t));
    }

    template <typename T>
    Size setSubData(Size index, const std::vector<T>& t) {
        if (t.empty()) {
            return 0;
        }
        Size offset = index * sizeof(T);
        Size size = t.size() * sizeof(T);
        return setSubData(offset, size, reinterpret_cast<const Byte*>(&t[0]));
    }

    // Append new data at the end of the current buffer
    // do a resize( size + getSize) and copy the new data
    // \return the number of bytes copied
    Size append(Size size, const Byte* data);

    template <typename T> 
    Size append(const T& t) {
        return append(sizeof(t), reinterpret_cast<const Byte*>(&t));
    }

    template <typename T>
    Size append(const std::vector<T>& t) {
        if (t.empty()) {
            return _end;
        }
        return append(sizeof(T) * t.size(), reinterpret_cast<const Byte*>(&t[0]));
    }


    const GPUObjectPointer gpuObject {};
    
    // Access the sysmem object, limited to ourselves and GPUObject derived classes
    const Sysmem& getSysmem() const { return _sysmem; }
    
    bool isDirty() const {
        return _pages(PageManager::DIRTY);
    }

    void applyUpdate(const Update& update);

    // Main thread operation to say that the buffer is ready to be used as a frame
    Update getUpdate() const;

protected:
    // For use by the render thread to avoid the intermediate step of getUpdate/applyUpdate
    void flush() const;

    // FIXME don't maintain a second buffer continuously.  We should be able to apply updates 
    // directly to the GL object and discard _renderSysmem and _renderPages
    mutable PageManager _renderPages;
    mutable Sysmem _renderSysmem;

    mutable std::atomic<size_t> _getUpdateCount { 0 };
    mutable std::atomic<size_t> _applyUpdateCount { 0 };

    void markDirty(Size offset, Size bytes);

    template <typename T>
    void markDirty(Size index, Size count = 1) {
        markDirty(sizeof(T) * index, sizeof(T) * count);
    }

    Sysmem& editSysmem() { return _sysmem; }
    Byte* editData() { return editSysmem().editData(); }

    mutable PageManager _pages;
    Size _end{ 0 };
    Sysmem _sysmem;


    friend class BufferView;
    friend class Frame;
    friend class Batch;

    // FIXME find a more generic way to do this.
    friend class gl::GLBackend;
    friend class gl::GLBuffer;
    friend class gl41::GL41Buffer;
    friend class gl45::GL45Buffer;
};

using BufferUpdates = std::vector<Buffer::Update>;

typedef std::shared_ptr<Buffer> BufferPointer;
typedef std::vector< BufferPointer > Buffers;


class BufferView {
protected:
    static const Resource::Size DEFAULT_OFFSET{ 0 };
    static const Element DEFAULT_ELEMENT;

public:
    using Size = Resource::Size;
    using Index = int;

    BufferPointer _buffer;
    Size _offset { 0 };
    Size _size { 0 };
    Element _element { DEFAULT_ELEMENT };
    uint16 _stride { 0 };

    BufferView(const BufferView& view) = default;
    BufferView& operator=(const BufferView& view) = default;

    BufferView();
    BufferView(const Element& element);
    BufferView(Buffer* newBuffer, const Element& element = DEFAULT_ELEMENT);
    BufferView(const BufferPointer& buffer, const Element& element = DEFAULT_ELEMENT);
    BufferView(const BufferPointer& buffer, Size offset, Size size, const Element& element = DEFAULT_ELEMENT);
    BufferView(const BufferPointer& buffer, Size offset, Size size, uint16 stride, const Element& element = DEFAULT_ELEMENT);

    Size getNumElements() const { return _size / _element.getSize(); }

    //Template iterator with random access on the buffer sysmem
    template<typename T>
    class Iterator : public std::iterator<std::random_access_iterator_tag, T, Index, T*, T&>
    {
    public:

        Iterator(T* ptr = NULL, int stride = sizeof(T)): _ptr(ptr), _stride(stride) { }
        Iterator(const Iterator<T>& iterator) = default;
        ~Iterator() {}

        Iterator<T>& operator=(const Iterator<T>& iterator) = default;
        Iterator<T>& operator=(T* ptr) {
            _ptr = ptr;
            // stride is left unchanged
            return (*this);
        }

        operator bool() const
        {
            if(_ptr)
                return true;
            else
                return false;
        }

        bool operator==(const Iterator<T>& iterator) const { return (_ptr == iterator.getConstPtr()); }
        bool operator!=(const Iterator<T>& iterator) const { return (_ptr != iterator.getConstPtr()); }

        void movePtr(const Index& movement) {
            auto byteptr = ((Byte*)_ptr);
            byteptr += _stride * movement;
            _ptr = (T*)byteptr;
        }

        Iterator<T>& operator+=(const Index& movement) {
            movePtr(movement);
            return (*this);
        }
        Iterator<T>& operator-=(const Index& movement) {
            movePtr(-movement);
            return (*this);
        }
        Iterator<T>& operator++() {
            movePtr(1);
            return (*this);
        }
        Iterator<T>& operator--() {
            movePtr(-1);
            return (*this);
        }
        Iterator<T> operator++(Index) {
            auto temp(*this);
            movePtr(1);
            return temp;
        }
        Iterator<T> operator--(Index) {
            auto temp(*this);
            movePtr(-1);
            return temp;
        }
        Iterator<T> operator+(const Index& movement) {
            auto oldPtr = _ptr;
            movePtr(movement);
            auto temp(*this);
            _ptr = oldPtr;
            return temp;
        }
        Iterator<T> operator-(const Index& movement) {
            auto oldPtr = _ptr;
            movePtr(-movement);
            auto temp(*this);
            _ptr = oldPtr;
            return temp;
        }

        Index operator-(const Iterator<T>& iterator) { return (iterator.getPtr() - this->getPtr())/sizeof(T); }

        T& operator*(){return *_ptr;}
        const T& operator*()const{return *_ptr;}
        T* operator->(){return _ptr;}

        T* getPtr()const{return _ptr;}
        const T* getConstPtr()const{return _ptr;}

    protected:

        T* _ptr;
        int _stride;
    };

#if 0
    // Direct memory access to the buffer contents is incompatible with the paging memory scheme
    template <typename T> Iterator<T> begin() { return Iterator<T>(&edit<T>(0), _stride); }
    template <typename T> Iterator<T> end() { return Iterator<T>(&edit<T>(getNum<T>()), _stride); }
#else 
    template <typename T> Iterator<const T> begin() const { return Iterator<const T>(&get<T>(), _stride); }
    template <typename T> Iterator<const T> end() const {
        // reimplement get<T> without bounds checking
        Resource::Size elementOffset = getNum<T>() * _stride + _offset;
        return Iterator<const T>((reinterpret_cast<const T*> (_buffer->getData() + elementOffset)), _stride);
    }
#endif
    template <typename T> Iterator<const T> cbegin() const { return Iterator<const T>(&get<T>(), _stride); }
    template <typename T> Iterator<const T> cend() const {
        // reimplement get<T> without bounds checking
        Resource::Size elementOffset = getNum<T>() * _stride + _offset;
        return Iterator<const T>((reinterpret_cast<const T*> (_buffer->getData() + elementOffset)), _stride);
    }

    // the number of elements of the specified type fitting in the view size
    template <typename T> Index getNum() const {
        return Index(_size / _stride);
    }

    template <typename T> const T& get() const {
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - _offset)) {
            qDebug() << "Accessing buffer in non allocated memory, element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - _offset);
        }
        if (sizeof(T) > _size) {
            qDebug() << "Accessing buffer outside the BufferView range, element size = " << sizeof(T) << " when bufferView size = " << _size;
        }
 #endif
        const T* t = (reinterpret_cast<const T*> (_buffer->getData() + _offset));
        return *(t);
    }

    template <typename T> T& edit() {
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - _offset)) {
            qDebug() << "Accessing buffer in non allocated memory, element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - _offset);
        }
        if (sizeof(T) > _size) {
            qDebug() << "Accessing buffer outside the BufferView range, element size = " << sizeof(T) << " when bufferView size = " << _size;
        }
 #endif
        _buffer->markDirty(_offset, sizeof(T));
        T* t = (reinterpret_cast<T*> (_buffer->editData() + _offset));
        return *(t);
    }

    template <typename T> const T& get(const Index index) const {
        Resource::Size elementOffset = index * _stride + _offset;
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - elementOffset)) {
            qDebug() << "Accessing buffer in non allocated memory, index = " << index << ", element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - elementOffset);
        }
        if (index > getNum<T>()) {
            qDebug() << "Accessing buffer outside the BufferView range, index = " << index << " number elements = " << getNum<T>();
        }
 #endif
        return *(reinterpret_cast<const T*> (_buffer->getData() + elementOffset));
    }

    template <typename T> T& edit(const Index index) const {
        Resource::Size elementOffset = index * _stride + _offset;
 #if _DEBUG
        if (!_buffer) {
            qDebug() << "Accessing null gpu::buffer!";
        }
        if (sizeof(T) > (_buffer->getSize() - elementOffset)) {
            qDebug() << "Accessing buffer in non allocated memory, index = " << index << ", element size = " << sizeof(T) << " available space in buffer at offset is = " << (_buffer->getSize() - elementOffset);
        }
        if (index > getNum<T>()) {
            qDebug() << "Accessing buffer outside the BufferView range, index = " << index << " number elements = " << getNum<T>();
        }
 #endif
        _buffer->markDirty(elementOffset, sizeof(T));
        return *(reinterpret_cast<T*> (_buffer->editData() + elementOffset));
    }
};
 
};

#endif
