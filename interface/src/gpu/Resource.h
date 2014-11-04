//
//  Resource.h
//  interface/src/gpu
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
#include "InterfaceConfig.h"

#include "gpu/Format.h"

#include <vector>

#include <QSharedPointer>

namespace gpu {

class GPUObject;

typedef int  Stamp;

class Resource {
public:
    typedef unsigned char Byte;
    typedef unsigned int  Size;

    static const Size NOT_ALLOCATED = -1;

    // The size in bytes of data stored in the resource
    virtual Size getSize() const = 0;

protected:

    Resource() {}
    virtual ~Resource() {}

    // Sysmem is the underneath cache for the data in ram of a resource.
    class Sysmem {
    public:

        Sysmem();
        Sysmem(Size size, const Byte* bytes);
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
        inline const Byte* readData() const { return _data; } 
        inline Byte* editData() { _stamp++; return _data; }

        template< typename T >
        const T* read() const { return reinterpret_cast< T* > ( _data ); } 
        template< typename T >
        T* edit() { _stamp++; return reinterpret_cast< T* > ( _data ); } 

        // Access the current version of the sysmem, used to compare if copies are in sync
        inline Stamp getStamp() const { return _stamp; }

        static Size allocateMemory(Byte** memAllocated, Size size);
        static void deallocateMemory(Byte* memDeallocated, Size size);

    private:
        Sysmem(const Sysmem& sysmem) {}
        Sysmem &operator=(const Sysmem& other) {return *this;}

        Stamp _stamp;
        Size  _size;
        Byte* _data;
    };

};

class Buffer : public Resource {
public:

    Buffer();
    Buffer(const Buffer& buf);
    ~Buffer();

    // The size in bytes of data stored in the buffer
    Size getSize() const { return getSysmem().getSize(); }
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

    // Append new data at the end of the current buffer
    // do a resize( size + getSize) and copy the new data
    // \return the number of bytes copied
    Size append(Size size, const Byte* data);

    // Access the sysmem object.
    const Sysmem& getSysmem() const { assert(_sysmem); return (*_sysmem); }


protected:

    Sysmem* _sysmem;

    mutable GPUObject* _gpuObject;

    Sysmem& editSysmem() { assert(_sysmem); return (*_sysmem); }

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject = gpuObject; }
    GPUObject* getGPUObject() const { return _gpuObject; }

    friend class Backend;
};

typedef QSharedPointer<Buffer> BufferPointer;
typedef std::vector< BufferPointer > Buffers;
};


#endif
