//
//  Format.h
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/29/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Format_h
#define hifi_gpu_Format_h

#include <assert.h>
#include "InterfaceConfig.h"


namespace gpu {

typedef unsigned int uint32;
typedef int int32;
typedef unsigned short uint16;
typedef short int16;
typedef unsigned char uint8;
typedef char int8;

typedef uint32 Offset;

// Description of a scalar type
enum Type {

    FLOAT = 0,
    INT32,
    UINT32,
    HALF,
    INT16,
    UINT16,
    INT8,
    UINT8,

    NFLOAT,
    NINT32,
    NUINT32,
    NHALF,
    NINT16,
    NUINT16,
    NINT8,
    NUINT8,

    NUM_TYPES,
};
// Array providing the size in bytes for a given scalar type
static const int TYPE_SIZE[NUM_TYPES] = {
    4,
    4,
    4,
    2,
    2,
    2,
    1,
    1,
    4,
    4,
    4,
    2,
    2,
    2,
    1,
    1
};


// Dimension of an Element
enum Dimension {
    SCALAR = 0,
    VEC2,
    VEC3,
    VEC4,
    MAT3,
    MAT4,

    NUM_DIMENSIONS,
};
// Count (of scalars) in an Element for a given Dimension
static const int DIMENSION_COUNT[NUM_DIMENSIONS] = {
    1,
    2,
    3,
    4,
    9,
    16
};

// Semantic of an Element
// Provide information on how to use the element
enum Semantic {
    RGB = 0,
    RGBA,
    XYZ,
    XYZW,
    POS_XYZ,
    POS_XYZW,
    QUAT,
    DIR_XYZ,
    UV,
    R8,

    NUM_SEMANTICS,
};

// Element is a simple 16bit value that contains everything we need to know about an element
// of a buffer, a pixel of a texture, a varying input/output or uniform from a shader pipeline.
// Type and dimension of the element, and semantic
class Element {
public:
    Element(Dimension dim, Type type, Semantic sem) :
        _semantic(sem),
        _dimension(dim),
        _type(type) 
    {}
    Element() :
        _semantic(R8),
        _dimension(SCALAR),
        _type(INT8)
    {}

    Semantic getSemantic() const { return (Semantic)_semantic; }

    Dimension getDimension() const { return (Dimension)_dimension; }
    uint8 getDimensionCount() const { return  DIMENSION_COUNT[(Dimension)_dimension]; }

    Type getType() const { return (Type)_type; }
    bool isNormalized() const { return (getType() >= NFLOAT); }

    uint32 getSize() const { return DIMENSION_COUNT[_dimension] * TYPE_SIZE[_type]; }

protected:
    uint8 _semantic;
    uint8 _dimension : 4;
    uint8 _type : 4;
};


};


#endif
