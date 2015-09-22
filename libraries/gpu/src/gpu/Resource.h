//
//  Resource.h
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 10/8/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Resource_h
#define hifi_gpu_Resource_h

#include <assert.h>

#include "Format.h"

#include <vector>

#include <memory>
#ifdef _DEBUG
#include <QDebug>
#endif

namespace gpu {

class Resource {
public:
    typedef unsigned int Size;

    static const Size NOT_ALLOCATED = -1;

    // The size in bytes of data stored in the resource
    virtual Size getSize() const = 0;

    enum Type {
        BUFFER = 0,
        TEXTURE_1D,
        TEXTURE_2D,
        TEXTURE_3D,
        TEXTURE_CUBE,
        TEXTURE_1D_ARRAY,
        TEXTURE_2D_ARRAY,
        TEXTURE_3D_ARRAY,
        TEXTURE_CUBE_ARRAY,
    };

protected:

    Resource() {}
    virtual ~Resource() {}

    // Sysmem is the underneath cache for the data in ram of a resource.
    class Sysmem {
    public:

        Sysmem();
        Sysmem(Size size, const Byte* bytes);
        Sysmem(const Sysmem& sysmem); // deep copy of the sysmem buffer
        Sysmem& operator=(const Sysmem& sysmem); // deep copy of the sysmem buffer
        ~Sysmem();

        Size getSize() const { return _size; }

        // Allocate the byte array
        // \param pSize The nb of bytes to allocate, if already exist, content is lost.
        // \return The nb of bytes allocated, nothing if allready the appropriate size.
        Size allocate(Size pSize);

        // Resize the byte array
        // Keep previous data [0 to min(pSize, mSize)]
        Size resize(Size pSize);

        // Assign data bytes and size (allocate for size, then copy bytes if exists)
        Size setData(Size size, const Byte* bytes );

        // Update Sub data, 
        // doesn't allocate and only copy size * bytes at the offset location
        // only if all fits in the existing allocated buffer
        Size setSubData(Size offset, Size size, const Byte* bytes);

        // Append new data at the end of the current buffer
        // do a resize( size + getSIze) and copy the new data
        // \return the number of bytes copied
        Size append(Size size, const Byte* data);

        // Access the byte array.
        // The edit version allow to map data.
        const Byte* readData() const { return _data; } 
        Byte* editData() { _stamp++; return _data; }

        template< typename T > const T* read() const { return reinterpret_cast< T* > ( _data ); } 
        template< typename T > T* edit() { _stamp++; return reinterpret_cast< T* > ( _data ); } 

        // Access the current version of the sysmem, used to compare if copies are in sync
        Stamp getStamp() const { return _stamp; }

        static Size allocateMemory(Byte** memAllocated, Size size);
        static void deallocateMemory(Byte* memDeallocated, Size size);

        bool isAvailable() const { return (_data != 0); }

    private:
        Stamp _stamp;
        Size  _size;
        Byte* _data;
    };

};

class Buffer : public Resource {
public:

    Buffer();
    Buffer(Size size, const Byte* bytes);
    Buffer(const Buffer& buf); // deep copy of the sysmem buffer
    Buffer& operator=(const Buffer& buf); // deep copy of the sysmem buffer
    ~Buffer();

    // The size in bytes of data stored in the buffer
    Size getSize() const { return getSysmem().getSize(); }
    const Byte* getData() const { return getSysmem().readData(); }
    Byte* editData() { return editSysmem().editData(); }

    // Resize the buffer
    // Keep previous data [0 to min(pSize, mSize)]
    Size resize(Size pSize);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    // \return the size of the buffer
    Size setData(Size size, const Byte* data);

    // Assign data bytes and size (allocate for size, then copy bytes if exists)
    // \return the number of bytes copied
    Size setSubData(Size offset, Size size, const Byte* data);

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
        return append(sizeof(T) * t.size(), reinterpret_cast<const Byte*>(&t[0]));
    }

    // Access the sysmem object.
    const Sysmem& getSysmem() const { assert(_sysmem); return (*_sysmem); }
    Sysmem& editSysmem() { assert(_sysmem); return (*_sysmem); }

protected:

    Sysmem* _sysmem = NULL;


    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObject* _gpuObject = NULL;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }
    friend class Backend;
};

typedef std::shared_ptr<Buffer> BufferPointer;
typedef std::vector< BufferPointer > Buffers;


class BufferView {
protected: 
    void initFromBuffer(const BufferPointer& buffer) {
        _buffer = (buffer);
        if (_buffer) {
            _size = (buffer->getSize());
        }
    }
public:
    typedef Resource::Size Size;
    typedef int Index;

    BufferPointer _buffer;
    Size _offset{ 0 };
    Size _size{ 0 };
    Element _element;
    uint16 _stride{ 1 };

    BufferView() :
        _buffer(NULL),
        _offset(0),
        _size(0),
        _element(gpu::SCALAR, gpu::UINT8, gpu::RAW),
        _stride(1)
    {};

    BufferView(const Element& element) :
        _buffer(NULL),
        _offset(0),
        _size(0),
        _element(element),
        _stride(uint16(element.getSize()))
    {};

    // create the BufferView and own the Buffer
    BufferView(Buffer* newBuffer, const Element& element = Element(gpu::SCALAR, gpu::UINT8, gpu::RAW)) :
        _offset(0),
        _element(element),
        _stride(uint16(element.getSize()))
    {
        initFromBuffer(BufferPointer(newBuffer));
    };
    BufferView(const BufferPointer& buffer, const Element& element = Element(gpu::SCALAR, gpu::UINT8, gpu::RAW)) :
        _offset(0),
        _element(element),
        _stride(uint16(element.getSize()))
    {
        initFromBuffer(buffer);
    };
    BufferView(const BufferPointer& buffer, Size offset, Size size, const Element& element = Element(gpu::SCALAR, gpu::UINT8, gpu::RAW)) :
        _buffer(buffer),
        _offset(offset),
        _size(size),
        _element(element),
        _stride(uint16(element.getSize()))
    {};
    BufferView(const BufferPointer& buffer, Size offset, Size size, uint16 stride, const Element& element = Element(gpu::SCALAR, gpu::UINT8, gpu::RAW)) :
        _buffer(buffer),
        _offset(offset),
        _size(size),
        _element(element),
        _stride(stride)
    {};

    ~BufferView() {}
    BufferView(const BufferView& view) = default;
    BufferView& operator=(const BufferView& view) = default;

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

    template <typename T> Iterator<T> begin() { return Iterator<T>(&edit<T>(0), _stride); }
    template <typename T> Iterator<T> end() { return Iterator<T>(&edit<T>(getNum<T>()), _stride); }
    template <typename T> Iterator<const T> cbegin() const { return Iterator<const T>(&get<T>(0), _stride); }
    template <typename T> Iterator<const T> cend() const { return Iterator<const T>(&get<T>(getNum<T>()), _stride); }

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
        return *(reinterpret_cast<T*> (_buffer->editData() + elementOffset));
    }
};
 
};

#endif
