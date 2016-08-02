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
#include <atomic>

#include <memory>
#ifdef _DEBUG
#include <QDebug>
#endif

namespace gpu {

// Sysmem is the underneath cache for the data in ram of a resource.
class Sysmem {
public:
    static const Size NOT_ALLOCATED = (Size)-1;

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
    Size setData(Size size, const Byte* bytes);

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
    Byte* editData() { return _data; }

    template< typename T > const T* read() const { return reinterpret_cast< T* > (_data); }
    template< typename T > T* edit() { return reinterpret_cast< T* > (_data); }

    // Access the current version of the sysmem, used to compare if copies are in sync
    Stamp getStamp() const { return _stamp; }

    static Size allocateMemory(Byte** memAllocated, Size size);
    static void deallocateMemory(Byte* memDeallocated, Size size);

    bool isAvailable() const { return (_data != 0); }

    using Operator = std::function<void(Sysmem& sysmem)>;
private:
    Stamp _stamp{ 0 };
    Size  _size{ 0 };
    Byte* _data{ nullptr };
}; // Sysmem

class Resource {
public:
    typedef size_t Size;

    static const Size NOT_ALLOCATED = Sysmem::NOT_ALLOCATED;

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

}; // Resource


struct PageManager {
    static const Size DEFAULT_PAGE_SIZE = 4096;

    enum Flag {
        DIRTY = 0x01,
    };

    PageManager(Size pageSize = DEFAULT_PAGE_SIZE) : _pageSize(pageSize) {}
    PageManager& operator=(const PageManager& other) {
        assert(other._pageSize == _pageSize);
        _pages = other._pages;
        _flags = other._flags;
        return *this;
    }

    using Vector = std::vector<uint8_t>;
    using Pages = std::vector<Size>;
    Vector _pages;

    uint8 _flags{ 0 };
    const Size _pageSize;

    operator bool() const {
        return (*this)(DIRTY);
    }

    bool operator()(uint8 desiredFlags) const {
        return (desiredFlags == (_flags & desiredFlags));
    }

    void markPage(Size index, uint8 markFlags = DIRTY) {
        assert(_pages.size() > index);
        _pages[index] |= markFlags;
        _flags |= markFlags;
    }

    void markRegion(Size offset, Size bytes, uint8 markFlags = DIRTY) {
        if (!bytes) {
            return;
        }
        _flags |= markFlags;
        // Find the starting page
        Size startPage = (offset / _pageSize);
        // Non-zero byte count, so at least one page is dirty
        Size pageCount = 1;
        // How much of the page is after the offset?
        Size remainder = _pageSize - (offset % _pageSize);
        //  If there are more bytes than page space remaining, we need to increase the page count
        if (bytes > remainder) {
            // Get rid of the amount that will fit in the current page
            bytes -= remainder;

            pageCount += (bytes / _pageSize);
            if (bytes % _pageSize) {
                ++pageCount;
            }
        }

        // Mark the pages dirty
        for (Size i = 0; i < pageCount; ++i) {
            _pages[i + startPage] |= DIRTY;
        }
    }

    Size getPageCount(uint8_t desiredFlags = DIRTY) const {
        Size result = 0;
        for (auto pageFlags : _pages) {
            if (desiredFlags == (pageFlags & desiredFlags)) {
                ++result;
            }
        }
        return result;
    }

    Size getSize(uint8_t desiredFlags = DIRTY) const {
        return getPageCount(desiredFlags) * _pageSize;
    }

    void setPageCount(Size count) {
        _pages.resize(count);
    }

    Size getRequiredPageCount(Size size) const {
        Size result = size / _pageSize;
        if (size % _pageSize) {
            ++result;
        }
        return result;
    }

    Size getRequiredSize(Size size) const {
        return getRequiredPageCount(size) * _pageSize;
    }

    Size accommodate(Size size) {
        Size newPageCount = getRequiredPageCount(size);
        Size newSize = newPageCount * _pageSize;
        _pages.resize(newPageCount, 0);
        return newSize;
    }

    // Get pages with the specified flags, optionally clearing the flags as we go
    Pages getMarkedPages(uint8_t desiredFlags = DIRTY, bool clear = true) {
        Pages result;
        if (desiredFlags == (_flags & desiredFlags)) {
            _flags &= ~desiredFlags;
            result.reserve(_pages.size());
            for (Size i = 0; i < _pages.size(); ++i) {
                if (desiredFlags == (_pages[i] & desiredFlags)) {
                    result.push_back(i);
                    if (clear) {
                        _pages[i] &= ~desiredFlags;
                    }
                }
            }
        }
        return result;
    }

    bool getNextTransferBlock(Size& outOffset, Size& outSize, Size& currentPage) {
        Size pageCount = _pages.size();
        // Advance to the first dirty page
        while (currentPage < pageCount && (0 == (DIRTY & _pages[currentPage]))) {
            ++currentPage;
        }

        // If we got to the end, we're done
        if (currentPage >= pageCount) {
            return false;
        }

        // Advance to the next clean page
        outOffset = static_cast<Size>(currentPage * _pageSize);
        while (currentPage < pageCount && (0 != (DIRTY & _pages[currentPage]))) {
            _pages[currentPage] &= ~DIRTY;
            ++currentPage;
        }
        outSize = static_cast<Size>((currentPage * _pageSize) - outOffset);
        return true;
    }
};


class Buffer : public Resource {
    static std::atomic<uint32_t> _bufferCPUCount;
    static std::atomic<Size> _bufferCPUMemoryUsage;
    static void updateBufferCPUMemoryUsage(Size prevObjectSize, Size newObjectSize);

public:
    using Flag = PageManager::Flag;
    struct Update {
        PageManager pages;
        Sysmem::Operator updateOperator;
    };

    // Currently only one flag... 'dirty'
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

    // For use by the render thread to avoid the intermediate step of getUpdate/applyUpdate
    void flush();

    // FIXME don't maintain a second buffer continuously.  We should be able to apply updates 
    // directly to the GL object and discard _renderSysmem and _renderPages
    mutable PageManager _renderPages;
    Sysmem _renderSysmem;

    mutable std::atomic<size_t> _getUpdateCount;
    mutable std::atomic<size_t> _applyUpdateCount;
protected:
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


    // FIXME find a more generic way to do this.
    friend class gl::GLBuffer;
    friend class BufferView;
    friend class Frame;
};

using BufferUpdate = std::pair<BufferPointer, Buffer::Update>;
using BufferUpdates = std::vector<BufferUpdate>;

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
    Size _offset;
    Size _size;
    Element _element;
    uint16 _stride;

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
    template <typename T> Iterator<const T> end() const { return Iterator<const T>(&get<T>(getNum<T>()), _stride); }
#endif
    template <typename T> Iterator<const T> cbegin() const { return Iterator<const T>(&get<T>(), _stride); }
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
