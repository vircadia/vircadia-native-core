//
//  PrimitiveRenderer.h
//  interface/src/voxels
//
//  Created by Norman Craft.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __interface__PrimitiveRenderer__
#define __interface__PrimitiveRenderer__

#include <QStack>
#include <QVector>
#include <QMutex>

/// Vertex element structure.
///    Using the array of structures approach to specifying
///    vertex data to GL cuts down on the calls to glBufferSubData
///
typedef
    struct __VertexElement {
        struct __position {
            float x;
            float y;
            float z;
        } position;
        struct __normal {
            float x;
            float y;
            float z;
        } normal;
        struct __color {
            unsigned char r;
            unsigned char g;
            unsigned char b;
            unsigned char a;
        } color;
    } VertexElement;

/// Triangle element index structure.
///    Specify the vertex indices of the triangle and its element index.
///
typedef 
    struct __TriElement {
        int indices[3];
        int id;

    } TriElement;

/// Vertex element list container.
///
typedef QVector<VertexElement *> VertexElementList;

/// Vertex element index list container.
///
typedef QVector<int> VertexElementIndexList;

/// Triangle element list container
///
typedef QVector<TriElement *> TriElementList;

/// 
///    @class Primitive
///    Primitive Interface class.
///    Abstract class for accessing vertex and tri elements of geometric primitives
///
///
class Primitive {
public:
    virtual ~Primitive();

    // API methods go here

    /// Vertex element accessor.
    ///    @return A list of vertex elements of the primitive
    ///
    const VertexElementList& vertexElements() const;

    /// Vertex element index accessor.
    ///    @return A list of vertex element indices of the primitive
    ///
    VertexElementIndexList& vertexElementIndices();

    /// Tri element accessor.
    ///    @return A list of tri elements of the primitive
    ///
    TriElementList& triElements();

    /// Release vertex elements.
    ///
    void releaseVertexElements();

    /// Get memory usage.
    ///
    unsigned long getMemoryUsage();

protected:
    /// Default constructor prohibited to API user, restricted to service implementer.
    ///
    Primitive();

private:
    /// Copy constructor prohibited.
    ///
    Primitive(
        const Primitive& prim
        );

    // SPI methods are defined here

    /// Vertex element accessor.
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///
    virtual const VertexElementList& vVertexElements() const = 0;

    /// Vertex element index accessor.
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///
    virtual VertexElementIndexList& vVertexElementIndices() = 0;

    /// Tri element accessor.
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///
    virtual TriElementList& vTriElements() = 0;

    /// Release vertex elements.
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///
    virtual void vReleaseVertexElements() = 0;

    /// Get memory usage.
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///
    virtual unsigned long vGetMemoryUsage() = 0;

};


/// 
///    @class Cube
///    Class for accessing the vertex and triangle elements of a cube
///
class Cube: public Primitive {
public:
    /// Configuration dependency injection constructor.
    ///
    Cube(
        float x,
        float y,
        float z,
        float s,
        unsigned char r,
        unsigned char g,
        unsigned char b,
        unsigned char faces
        );

    ~Cube();

private:
    /// Copy constructor prohibited.
    ///
    Cube (
        const Cube& cube
        );

    void init(
        float x,
        float y,
        float z,
        float s,
        unsigned char r,
        unsigned char g,
        unsigned char b,
        unsigned char faceExclusions
        );

    void terminate();

    void initializeVertices(
        float x,
        float y,
        float z,
        float s,
        unsigned char r,
        unsigned char g,
        unsigned char b,
        unsigned char faceExclusions
        );

    void terminateVertices();

    void initializeTris(
        unsigned char faceExclusions
        );

    void terminateTris();

    // SPI virtual override methods go here

    const VertexElementList& vVertexElements() const;
    VertexElementIndexList& vVertexElementIndices();
    TriElementList& vTriElements();
    void vReleaseVertexElements();
    unsigned long vGetMemoryUsage();

private:
    VertexElementList _vertices;                ///< Vertex element list
    VertexElementIndexList _vertexIndices;      ///< Vertex element index list
    TriElementList _tris;                       ///< Tri element list

    unsigned long _cpuMemoryUsage;              ///< Memory allocation of object

    static const int _sNumFacesPerCube = 6;
    static const int _sNumVerticesPerCube = 24;
    static unsigned char _sFaceIndexToHalfSpaceMask[6];
    static float _sVertexIndexToConstructionVector[24][3];
    static float _sVertexIndexToNormalVector[6][3];

};


/// 
///    @class Renderer
///    GL renderer interface class.
///    Abstract class for rendering geometric primitives in GL
///
class Renderer {
public:
    virtual ~Renderer();

    // API methods go here

    /// Add primitive to renderer database.
    ///
    int add(
        Primitive* primitive                    ///< Pointer to primitive
        );

    /// Remove primitive from renderer database.
    ///
    void remove(
        int id                                    ///< Primitive id
        );

    /// Clear all primitives from renderer database
    ///
    void release();

    /// Render primitive database.
    ///    The render method assumes appropriate GL context and state has
    ///    already been provided for
    ///
    void render();

    /// Get memory usage.
    ///
    unsigned long getMemoryUsage();

    /// Get GPU memory usage.
    ///
    unsigned long getMemoryUsageGPU();

protected:
    /// Default constructor prohibited to API user, restricted to service implementer.
    ///
    Renderer();

private:
    /// Copy constructor prohibited.
    ///
    Renderer(
        const Renderer& primitive
        );

    // SPI methods are defined here

    /// Add primitive to renderer database.
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///    @return primitive id
    ///
    virtual int vAdd(
        Primitive* primitive                    ///< Pointer to primitive
        ) = 0;

    /// Remove primitive from renderer database.
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///
    virtual void vRemove(
        int id                                    ///< Primitive id
        ) = 0;

    /// Clear all primitives from renderer database
    ///    Service implementer to provide private override for this method
    ///    in derived class
    ///
    virtual void vRelease() = 0;

    /// Render primitive database.
    ///    Service implementer to provide private virtual override for this method
    ///    in derived class
    ///
    virtual void vRender() = 0;

    /// Get memory usage.
    ///
    virtual unsigned long vGetMemoryUsage() = 0;

    /// Get GPU memory usage.
    ///
    virtual unsigned long vGetMemoryUsageGPU() = 0;

};

///
///    @class PrimitiveRenderer
///    Renderer implementation class for the rendering of geometric primitives
///    using GL element array and GL array buffers
///
class PrimitiveRenderer : public Renderer {
public:
    /// Configuration dependency injection constructor.
    ///
    PrimitiveRenderer(
        int maxCount
        );

    ~PrimitiveRenderer();

private:
    /// Default constructor prohibited.
    ///
    PrimitiveRenderer();

    /// Copy constructor prohibited.
    ///
    PrimitiveRenderer(
        const PrimitiveRenderer& renderer
        );

    void init();
    void terminate();

    /// Allocate and initialize GL buffers.
    ///
    void initializeGL();

    /// Terminate and deallocate GL buffers.
    ///
    void terminateGL();

    void initializeBookkeeping();
    void terminateBookkeeping();

    /// Construct the elements of the faces of the primitive.
    ///
    void constructElements(
        Primitive* primitive
        );

    /// Deconstruct the elements of the faces of the primitive.
    ///
    void deconstructElements(
        Primitive* primitive
        );

    /// Deconstruct the triangle element from the GL buffer.
    ///
    void deconstructTriElement(
        int idx
        );

    /// Deconstruct the vertex element from the GL buffer.
    ///
    void deconstructVertexElement(
        int idx
        );

    /// Transfer the vertex element to the GL buffer.
    ///
    void transferVertexElement(
        int idx,
        VertexElement *vertex
        );

    /// Transfer the triangle element to the GL buffer.
    ///
    void transferTriElement(
        int idx,
        int tri[3]
        );

    /// Get available primitive index.
    ///    Get an available primitive index from either the recycling
    ///    queue or incrementing the counter
    ///
    int getAvailablePrimitiveIndex();

    /// Get available vertex element index.
    ///    Get an available vertex element index from either the recycling
    ///    queue or incrementing the counter
    ///
    int getAvailableVertexElementIndex();

    /// Get available triangle element index.
    ///    Get an available triangle element index from either the elements
    ///    scheduled for deconstruction queue, the recycling
    ///    queue or incrementing the counter
    ///
    int getAvailableTriElementIndex();

    // SPI virtual override methods go here

    /// Add primitive to renderer database.
    ///
    int vAdd(
        Primitive* primitive
        );

    /// Remove primitive from renderer database.
    ///
    void vRemove(
        int id
        );

    /// Clear all primitives from renderer database
    ///
    void vRelease();

    /// Render triangle database.
    ///
    void vRender();

    /// Get memory usage.
    ///
    unsigned long vGetMemoryUsage();

    /// Get gpu memory usage.
    ///
    unsigned long vGetMemoryUsageGPU();

private:

    int _maxCount;

    // GL related parameters

    GLuint _triBufferId;                        ///< GL element array buffer id
    GLuint _vertexBufferId;                     ///< GL vertex array buffer id

    // Book keeping parameters

    int _vertexElementCount;                    ///< Count of vertices
    int _maxVertexElementCount;                 ///< Max count of vertices

    int _triElementCount;                       ///< Count of triangles
    int _maxTriElementCount;                    ///< Max count of triangles

    QVector<Primitive *> _primitives;           ///< Vector of primitive
    int _primitiveCount;                        ///< Count of primitives

    QStack<int> _availablePrimitiveIndex;       ///< Queue of primitive indices available
    QStack<int> _availableVertexElementIndex;   ///< Queue of vertex element indices available
    QStack<int> _availableTriElementIndex;      ///< Queue of triangle element indices available
    QStack<int> _deconstructTriElementIndex;    ///< Queue of triangle element indices requiring deletion from GL
    QStack<int> _constructPrimitiveIndex;       ///< Queue of primitives requiring addition to GL

    QMutex _guard;

    // Statistics parameters, not necessary for proper operation

    unsigned long _gpuMemoryUsage;
    unsigned long _cpuMemoryUsage;


    static const int _sIndicesPerTri = 3;
    static const int _sBytesPerTriElement = sizeof(GLint) * _sIndicesPerTri;
};


#endif
