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

// Description of a scalar type
enum Type : uint8_t {

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
    NUINT2,
    NINT2_10_10_10,

    COMPRESSED,

    NUM_TYPES,

    BOOL = UINT8,
    NORMALIZED_START = NINT32,
};
// Array providing the size in bytes for a given scalar type
static const int TYPE_SIZE[NUM_TYPES] = {
    4, // FLOAT
    4, // INT32
    4, // UINT32
    2, // HALF
    2, // INT16
    2, // UINT16
    1, // INT8
    1, // UINT8

    // normalized values
    4, // NINT32
    4, // NUINT32
    2, // NINT16 
    2, // NUINT16
    1, // NINT8
    1, // NUINT8
    1, // NUINT2
    1, // NINT2_10_10_10

    1, // COMPRESSED
};

// Array answering the question Does this type is integer or not 
static const bool TYPE_IS_INTEGER[NUM_TYPES] = {
    false, // FLOAT
    true, // INT32
    true, // UINT32
    false, // HALF
    true, // INT16
    true, // UINT16
    true, // INT8
    true, // UINT8

    // Normalized values
    false, // NINT32
    false, // NUINT32
    false, // NINT16 
    false, // NUINT16
    false, // NINT8
    false, // NUINT8
    false, // NUINT2
    false, // NINT2_10_10_10

    false, // COMPRESSED
};

// Dimension of an Element
enum Dimension : uint8_t {
    SCALAR = 0,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4,
    TILE4x4, // Blob element's size is defined from the type and semantic, it s counted as 1 component
    NUM_DIMENSIONS,
};

// Count (of scalars) in an Element for a given Dimension
static const int DIMENSION_LOCATION_COUNT[NUM_DIMENSIONS] = {
    1,
    1,
    1,
    1,
    1,
    3,
    4,
    1,
};

// Count (of scalars) in an Element for a given Dimension's location
static const int DIMENSION_SCALAR_COUNT_PER_LOCATION[NUM_DIMENSIONS] = {
    1,
    2,
    3,
    4,
    4,
    3,
    4,
    1,
};

// Count (of scalars) in an Element for a given Dimension
static const int DIMENSION_SCALAR_COUNT[NUM_DIMENSIONS] = {
    1,
    2,
    3,
    4,
    4,
    9,
    16,
    1,
};

// Tile dimension described by the ELement for "Tilexxx" DIMENSIONs
static const glm::ivec2 DIMENSION_TILE_DIM[NUM_DIMENSIONS] = {
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 1, 1 },
    { 4, 4 },
};

// Semantic of an Element
// Provide information on how to use the element
enum Semantic : uint8_t {
    RAW = 0, // used as RAW memory

    RED,
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

    SRED,
    SRGB,
    SRGBA,
    SBGRA,

    // These are generic compression format smeantic for images
    // THey must be used with Dim = BLOB and Type = Compressed
    // THe size of a compressed element is defined from the semantic
    _FIRST_COMPRESSED,

    COMPRESSED_BC1_SRGB,
    COMPRESSED_BC1_SRGBA,
    COMPRESSED_BC3_SRGBA,
    COMPRESSED_BC4_RED,
    COMPRESSED_BC5_XY,
    COMPRESSED_BC6_RGB,
    COMPRESSED_BC7_SRGBA,

    COMPRESSED_ETC2_RGB,
    COMPRESSED_ETC2_SRGB,
    COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA,
    COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA,
    COMPRESSED_ETC2_RGBA,
    COMPRESSED_ETC2_SRGBA,
    COMPRESSED_EAC_RED,
    COMPRESSED_EAC_RED_SIGNED,
    COMPRESSED_EAC_XY,
    COMPRESSED_EAC_XY_SIGNED,

    _LAST_COMPRESSED,

    R11G11B10,
    RGB9E5,

    UNIFORM,
    UNIFORM_BUFFER,
    RESOURCE_BUFFER,
    SAMPLER,
    SAMPLER_MULTISAMPLE,
    SAMPLER_SHADOW,


    NUM_SEMANTICS, // total Number of semantics (not a valid Semantic)!
};

// Array providing the scaling factor to size in bytes depending on a given semantic
static const int SEMANTIC_SIZE_FACTOR[NUM_SEMANTICS] = {
    1, //RAW = 0, // used as RAW memory

    1, //RED,
    1, //RGB,
    1, //RGBA,
    1, //BGRA,

    1, //XY,
    1, //XYZ,
    1, //XYZW,
    1, //QUAT,
    1, //UV,
    1, //INDEX, //used by index buffer of a mesh
    1, //PART, // used by part buffer of a mesh

    1, //DEPTH, // Depth only buffer
    1, //STENCIL, // Stencil only buffer
    1, //DEPTH_STENCIL, // Depth Stencil buffer

    1, //SRED,
    1, //SRGB,
    1, //SRGBA,
    1, //SBGRA,

    // These are generic compression format smeantic for images
    // THey must be used with Dim = BLOB and Type = Compressed
    // THe size of a compressed element is defined from the semantic
    1, //_FIRST_COMPRESSED,

    8, //COMPRESSED_BC1_SRGB, 1/2 byte/pixel * 4x4 pixels = 8 bytes
    8, //COMPRESSED_BC1_SRGBA, 1/2 byte/pixel * 4x4 pixels = 8 bytes
    16, //COMPRESSED_BC3_SRGBA, 1 byte/pixel * 4x4 pixels = 16 bytes
    8, //COMPRESSED_BC4_RED, 1/2 byte/pixel * 4x4 pixels = 8 bytes
    16, //COMPRESSED_BC5_XY, 1 byte/pixel * 4x4 pixels = 16 bytes
    16, //COMPRESSED_BC6_RGB, 1 byte/pixel * 4x4 pixels = 16 bytes
    16, //COMPRESSED_BC7_SRGBA, 1 byte/pixel * 4x4 pixels = 16 bytes

    8,  //COMPRESSED_ETC2_RGB,
    8,  //COMPRESSED_ETC2_SRGB,
    8,  //COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA,
    8,  //COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA,
    16, //COMPRESSED_ETC2_RGBA,
    16, //COMPRESSED_ETC2_SRGBA,
    8,  //COMPRESSED_EAC_RED,
    8,  //COMPRESSED_EAC_RED_SIGNED,
    16, //COMPRESSED_EAC_XY,
    16, //COMPRESSED_EAC_XY_SIGNED,

    1, //_LAST_COMPRESSED,

    1, //R11G11B10,
    1, //RGB9E5

    1, //UNIFORM,
    1, //UNIFORM_BUFFER,
    1, //RESOURCE_BUFFER,
    1, //SAMPLER,
    1, //SAMPLER_MULTISAMPLE,
    1, //SAMPLER_SHADOW,
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

    uint8 getScalarCount() const { return DIMENSION_SCALAR_COUNT[(Dimension)_dimension]; }
    uint32 getSize() const { return (DIMENSION_SCALAR_COUNT[_dimension] * TYPE_SIZE[_type] * SEMANTIC_SIZE_FACTOR[_semantic]); }
    const glm::ivec2& getTile() const { return (DIMENSION_TILE_DIM[_dimension]); }

    uint8 getLocationCount() const { return  DIMENSION_LOCATION_COUNT[(Dimension)_dimension]; }
    uint8 getLocationScalarCount() const { return DIMENSION_SCALAR_COUNT_PER_LOCATION[(Dimension)_dimension]; }
    uint32 getLocationSize() const { return DIMENSION_SCALAR_COUNT_PER_LOCATION[_dimension] * TYPE_SIZE[_type]; }

    uint16 getRaw() const { return *((const uint16*) (this)); }

    
    bool operator ==(const Element& right) const {
        return getRaw() == right.getRaw();
    }
    bool operator !=(const Element& right) const {
        return getRaw() != right.getRaw();
    }

    static const Element COLOR_R_8;
    static const Element COLOR_SR_8;
    static const Element COLOR_RGBA_32;
    static const Element COLOR_SRGBA_32;
    static const Element COLOR_BGRA_32;
    static const Element COLOR_SBGRA_32;
    static const Element COLOR_RGBA_2;
    static const Element COLOR_R11G11B10;
    static const Element COLOR_RGB9E5;
    static const Element COLOR_COMPRESSED_BCX_RED;
    static const Element COLOR_COMPRESSED_BCX_SRGB;
    static const Element COLOR_COMPRESSED_BCX_SRGBA_MASK;
    static const Element COLOR_COMPRESSED_BCX_SRGBA;
    static const Element COLOR_COMPRESSED_BCX_XY;
    static const Element COLOR_COMPRESSED_BCX_SRGBA_HIGH;
    static const Element COLOR_COMPRESSED_BCX_HDR_RGB;
    static const Element COLOR_COMPRESSED_ETC2_RGB;
    static const Element COLOR_COMPRESSED_ETC2_SRGB;
    static const Element COLOR_COMPRESSED_ETC2_RGB_PUNCHTHROUGH_ALPHA;
    static const Element COLOR_COMPRESSED_ETC2_SRGB_PUNCHTHROUGH_ALPHA;
    static const Element COLOR_COMPRESSED_ETC2_RGBA;
    static const Element COLOR_COMPRESSED_ETC2_SRGBA;
    static const Element COLOR_COMPRESSED_EAC_RED;
    static const Element COLOR_COMPRESSED_EAC_RED_SIGNED;
    static const Element COLOR_COMPRESSED_EAC_XY;
    static const Element COLOR_COMPRESSED_EAC_XY_SIGNED;
    static const Element DEPTH24_STENCIL8;
    static const Element VEC2NU8_XY;
    static const Element VEC4F_COLOR_RGBA;
    static const Element VEC2F_UV;
    static const Element VEC2F_XY;
    static const Element VEC3F_XYZ;
    static const Element VEC4F_XYZW;
    static const Element VEC4F_NORMALIZED_XYZ10W2;
    static const Element INDEX_UINT16;
    static const Element INDEX_INT32;
    static const Element PART_DRAWCALL;
    
 protected:
    uint16 _semantic : 7;
    uint16 _dimension : 4;
    uint16 _type : 5;
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
