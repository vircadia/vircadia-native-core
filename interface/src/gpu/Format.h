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

// Format is a simple 32bit value that contains everything we need to know about an element
// of a buffer, a pixel of a texture, a varying input/output or uniform from a shader pipeline.
// Type and dimension of the element, and semantic
class Element {
public:

    enum Type {

        TYPE_FLOAT = 0,
        TYPE_INT32,
        TYPE_UINT32,
        TYPE_HALF,
        TYPE_INT16,
        TYPE_UINT16,
        TYPE_INT8,
        TYPE_UINT8,

        TYPE_NFLOAT,
        TYPE_NINT32,
        TYPE_NUINT32,
        TYPE_NHALF,
        TYPE_NINT16,
        TYPE_NUINT16,
        TYPE_NINT8,
        TYPE_NUINT8,

        NUM_TYPES,
    };
    static const int TYPE_SIZE[NUM_TYPES];

    enum Dimension {
        DIM_SCALAR = 0,
        DIM_VEC2,
        DIM_VEC3,
        DIM_VEC4,
        DIM_MAT3,
        DIM_MAT4,

        NUM_DIMENSIONS,
    };
    static const int DIMENSION_COUNT[NUM_DIMENSIONS];

    enum Semantic {
        SEMANTIC_RGB = 0,
        SEMANTIC_RGBA,
        SEMANTIC_XYZ,
        SEMANTIC_XYZW,
        SEMANTIC_POS_XYZ,
        SEMANTIC_POS_XYZW,
        SEMANTIC_QUAT,
        SEMANTIC_DIR_XYZ,
        SEMANTIC_UV,
        SEMANTIC_R8,

        NUM_SEMANTICS,
    };

    Element(Dimension dim, Type type, Semantic sem) :
        _semantic(sem),
        _dimension(dim),
        _type(type) 
    {}
    Element() :
        _semantic(SEMANTIC_R8),
        _dimension(DIM_SCALAR),
        _type(TYPE_INT8)
    {}

    Semantic getSemantic() const { return (Semantic)_semantic; }

    Dimension getDimension() const { return (Dimension)_dimension; }
    uint8 getDimensionCount() const { return  DIMENSION_COUNT[(Dimension)_dimension]; }

    Type getType() const { return (Type)_type; }
    bool isNormalized() const { return (getType() >= TYPE_NFLOAT); }

    uint32 getSize() const { return DIMENSION_COUNT[_dimension] * TYPE_SIZE[_type]; }

protected:
    uint8 _semantic;
    uint8 _dimension : 4;
    uint8 _type : 4;
};


};


#endif
