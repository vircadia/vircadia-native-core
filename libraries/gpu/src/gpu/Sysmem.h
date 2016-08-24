//
//  Created by Sam Gateau on 10/8/2014.
//  Split from Resource.h/Resource.cpp by Bradley Austin Davis on 2016/08/07
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Sysmem_h
#define hifi_gpu_Sysmem_h


#include "Forward.h"

namespace gpu {

// Sysmem is the underneath cache for the data in ram of a resource.
class Sysmem {
public:
    static const Size NOT_ALLOCATED = INVALID_SIZE;

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

private:
    Stamp _stamp{ 0 };
    Size  _size{ 0 };
    Byte* _data{ nullptr };
}; // Sysmem

}

#endif
