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
#include <memory>

#include "Forward.h"

namespace gpu {

class Backend;

class GPUObject {
public:
    virtual ~GPUObject() = default;
};

class GPUObjectPointer {
private:
    using GPUObjectUniquePointer = std::unique_ptr<GPUObject>;

    // This shouldn't be used by anything else than the Backend class with the proper casting.
    mutable GPUObjectUniquePointer _gpuObject;
    void setGPUObject(GPUObject* gpuObject) const { _gpuObject.reset(gpuObject); }
    GPUObject* getGPUObject() const { return _gpuObject.get(); }

    friend class Backend;
};

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

    NINT32,
    NUINT32,
    NINT16,
    NUINT16,
    NINT8,
    NUINT8,

    NUM_TYPES,

    BOOL = UINT8,
    NORMALIZED_START = NINT32,
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

    // normalized values
    4,
    4,
    2,
    2,
    1,
    1
};
// Array answering the question Does this type is integer or not 
static const bool TYPE_IS_INTEGER[NUM_TYPES] = {
    false,
    true,
    true,
    false,
    true,
    true,
    true,
    true,

    // Normalized values
    true,
    true,
    true,
    true,
    true,
    true
};

// Dimension of an Element
enum Dimension {
    SCALAR = 0,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4,
    NUM_DIMENSIONS,
};

// Count (of scalars) in an Element for a given Dimension
static const int LOCATION_COUNT[NUM_DIMENSIONS] = {
    1,
    1,
    1,
    1,
    1,
    3,
    4,
};

// Count (of scalars) in an Element for a given Dimension's location
static const int SCALAR_COUNT_PER_LOCATION[NUM_DIMENSIONS] = {
    1,
    2,
    3,
    4,
    4,
    3,
    4,
};

// Count (of scalars) in an Element for a given Dimension
static const int SCALAR_COUNT[NUM_DIMENSIONS] = {
    1,
    2,
    3,
    4,
    4,
    9,
    16,
};

// Semantic of an Element
// Provide information on how to use the element
enum Semantic {
    RAW = 0, // used as RAW memory

    RGB,
    RGBA,
    BGRA,

    XY,
    XYZ,
    XYZW,
    QUAT,
    UV,
    INDEX, //used by index buffer of a mesh
    PART, // used by part buffer of a mesh

    DEPTH, // Depth only buffer
    STENCIL, // Stencil only buffer
    DEPTH_STENCIL, // Depth Stencil buffer

    SRGB,
    SRGBA,
    SBGRA,

    // These are generic compression format smeantic for images
    _FIRST_COMPRESSED,
    COMPRESSED_R,

    COMPRESSED_RGB, 
    COMPRESSED_RGBA,

    COMPRESSED_SRGB,
    COMPRESSED_SRGBA,

    // FIXME: Will have to be supported later:
    /*COMPRESSED_BC3_RGBA,  // RGBA_S3TC_DXT5_EXT,
    COMPRESSED_BC3_SRGBA, // SRGB_ALPHA_S3TC_DXT5_EXT

    COMPRESSED_BC7_RGBA,
    COMPRESSED_BC7_SRGBA, */

    _LAST_COMPRESSED,

    R11G11B10,

    UNIFORM,
    UNIFORM_BUFFER,
    SAMPLER,
    SAMPLER_MULTISAMPLE,
    SAMPLER_SHADOW,
  
 
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
        _semantic(RAW),
        _dimension(SCALAR),
        _type(INT8)
    {}
 
    Semantic getSemantic() const { return (Semantic)_semantic; }

    Dimension getDimension() const { return (Dimension)_dimension; }
    
    bool isCompressed() const { return uint8(getSemantic() - _FIRST_COMPRESSED) <= uint8(_LAST_COMPRESSED - _FIRST_COMPRESSED); }

    Type getType() const { return (Type)_type; }
    bool isNormalized() const { return (getType() >= NORMALIZED_START); }
    bool isInteger() const { return TYPE_IS_INTEGER[getType()]; }

    uint8 getScalarCount() const { return  SCALAR_COUNT[(Dimension)_dimension]; }
    uint32 getSize() const { return SCALAR_COUNT[_dimension] * TYPE_SIZE[_type]; }

    uint8 getLocationCount() const { return  LOCATION_COUNT[(Dimension)_dimension]; }
    uint8 getLocationScalarCount() const { return  SCALAR_COUNT_PER_LOCATION[(Dimension)_dimension]; }
    uint32 getLocationSize() const { return SCALAR_COUNT_PER_LOCATION[_dimension] * TYPE_SIZE[_type]; }

    uint16 getRaw() const { return *((uint16*) (this)); }

    
    bool operator ==(const Element& right) const {
        return getRaw() == right.getRaw();
    }
    bool operator !=(const Element& right) const {
        return getRaw() != right.getRaw();
    }

    static const Element COLOR_RGBA_32;
    static const Element COLOR_SRGBA_32;
    static const Element COLOR_R11G11B10;
    static const Element VEC4F_COLOR_RGBA;
    static const Element VEC2F_UV;
    static const Element VEC2F_XY;
    static const Element VEC3F_XYZ;
    static const Element VEC4F_XYZW;
    static const Element INDEX_UINT16;
    static const Element PART_DRAWCALL;
    
 protected:
    uint8 _semantic;
    uint8 _dimension : 4;
    uint8 _type : 4;
};

  
enum ComparisonFunction {
    NEVER = 0,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS,

    NUM_COMPARISON_FUNCS,
};

enum Primitive {
    POINTS = 0,
    LINES,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    NUM_PRIMITIVES,
};

};


#endif
