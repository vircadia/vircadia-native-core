/// 
///    @file PrimitiveRenderer.cpp
///    A geometric primitive renderer.
///
///    @author: Norman Crafts
///    @copyright 2014, High Fidelity, Inc. All rights reserved.
///

#include <QMutexLocker>

#include "InterfaceConfig.h"
#include "OctreeElement.h"
#include "PrimitiveRenderer.h"

Primitive::Primitive() {
}

Primitive::~Primitive() {
}

// Simple dispatch between API and SPI

const VertexElementList& Primitive::vertexElements() const {
    return vVertexElements();
}

VertexElementIndexList& Primitive::vertexElementIndices() {
    return vVertexElementIndices();
}

TriElementList& Primitive::triElements() {
    return vTriElements();
}

void Primitive::releaseVertexElements() {
    vReleaseVertexElements();
}

unsigned long Primitive::getMemoryUsage() {
    return vGetMemoryUsage();
}


Cube::Cube(
    float x,
    float y,
    float z,
    float s,
    unsigned char r,
    unsigned char g,
    unsigned char b,
    unsigned char faceExclusions
    ) :
        _cpuMemoryUsage(0) {
    init(x, y, z, s, r, g, b, faceExclusions);
}

Cube::~Cube() {
    terminate();
}

void Cube::init(
    float x,
    float y,
    float z,
    float s,
    unsigned char r,
    unsigned char g,
    unsigned char b,
    unsigned char faceExclusions
    ) {

    initializeVertices(x, y, z, s, r, g, b, faceExclusions);
    initializeTris(faceExclusions);
}

void Cube::terminate() {

    terminateTris();
    terminateVertices();
}

void Cube::initializeVertices(
    float x,
    float y,
    float z,
    float s,
    unsigned char r,
    unsigned char g,
    unsigned char b,
    unsigned char faceExclusions
    ) {

    for (int i = 0; i < _sNumVerticesPerCube; i++) {
        // Check whether the vertex is necessary for the faces indicated by faceExclusions bit mask.
        // uncomment this line to load all faces: if (~0x00 & _sFaceIndexToHalfSpaceMask[i >> 2]) {
        // uncomment this line to include shared faces: if (faceExclusions & _sFaceIndexToHalfSpaceMask[i >> 2]) {
        // uncomment this line to exclude shared faces: 
        if (~faceExclusions & _sFaceIndexToHalfSpaceMask[i >> 2]) {

            VertexElement* v = new VertexElement();
            if (v) {
                // Construct vertex position
                v->position.x = x + s * _sVertexIndexToConstructionVector[i][0];
                v->position.y = y + s * _sVertexIndexToConstructionVector[i][1];
                v->position.z = z + s * _sVertexIndexToConstructionVector[i][2];

                // Construct vertex normal
                v->normal.x = _sVertexIndexToNormalVector[i >> 2][0];
                v->normal.y = _sVertexIndexToNormalVector[i >> 2][1];
                v->normal.z = _sVertexIndexToNormalVector[i >> 2][2];

                // Construct vertex color
//#define FALSE_COLOR
#ifndef FALSE_COLOR
                v->color.r = r;
                v->color.g = g;
                v->color.b = b;
                v->color.a = 255;
#else
                static unsigned char falseColor[6][3] = {
                    192,   0,   0, // Bot
                      0, 192,   0, // Top
                      0,   0, 192, // Right
                    192,   0, 192, // Left
                    192, 192,   0, // Near
                    192, 192, 192  // Far
                }; 
                v->color.r = falseColor[i >> 2][0];
                v->color.g = falseColor[i >> 2][1];
                v->color.b = falseColor[i >> 2][2];
                v->color.a = 255;
#endif

                // Add vertex element to list
                _vertices.push_back(v);
                _cpuMemoryUsage += sizeof(VertexElement);
                _cpuMemoryUsage += sizeof(VertexElement*);
            }
        }
    }
}

void Cube::terminateVertices() {

    for (VertexElementList::iterator it = _vertices.begin(); it != _vertices.end(); ++it) {
        delete *it;
    }
    _cpuMemoryUsage -= _vertices.size() * (sizeof(VertexElement) + sizeof(VertexElement*));
    _vertices.clear();
}

void Cube::initializeTris(
    unsigned char faceExclusions
    ) {

    int index = 0;
    for (int i = 0; i < _sNumFacesPerCube; i++) {
        // Check whether the vertex is necessary for the faces indicated by faceExclusions bit mask.
        // uncomment this line to load all faces: if (~0x00 & _sFaceIndexToHalfSpaceMask[i]) {
        // uncomment this line to include shared faces: if (faceExclusions & _sFaceIndexToHalfSpaceMask[i]) {
        // uncomment this line to exclude shared faces: 
        if (~faceExclusions & _sFaceIndexToHalfSpaceMask[i]) {
            
            int start = index;
            // Create the triangulated face, two tris, six indices referencing four vertices, both
            // with cw winding order, such that:

            //       A-B
            //       |\|
            //       D-C

            // Store triangle ABC

            TriElement* tri = new TriElement();
            if (tri) {
                tri->indices[0] = index++;
                tri->indices[1] = index++;
                tri->indices[2] = index;

                // Add tri element to list
                _tris.push_back(tri);
                _cpuMemoryUsage += sizeof(TriElement);
                _cpuMemoryUsage += sizeof(TriElement*);
            }

            // Now store triangle ACD
            tri = new TriElement();
            if (tri) {
                tri->indices[0] = start;
                tri->indices[1] = index++;
                tri->indices[2] = index++;

                // Add tri element to list
                _tris.push_back(tri);
                _cpuMemoryUsage += sizeof(TriElement);
                _cpuMemoryUsage += sizeof(TriElement*);
            }
        }
    }
}

void Cube::terminateTris() {

    for (TriElementList::iterator it = _tris.begin(); it != _tris.end(); ++it) {
        delete *it;
    }
    _cpuMemoryUsage -= _tris.size() * (sizeof(TriElement) + sizeof(TriElement*));
    _tris.clear();
}

const VertexElementList& Cube::vVertexElements() const {
    return _vertices;
}

VertexElementIndexList& Cube::vVertexElementIndices() {
    return _vertexIndices;
}

TriElementList& Cube::vTriElements() {
    return _tris;
}

void Cube::vReleaseVertexElements() {
    terminateVertices();
}

unsigned long Cube::vGetMemoryUsage() {
    return _cpuMemoryUsage;
}

unsigned char Cube::_sFaceIndexToHalfSpaceMask[6] = {
    OctreeElement::HalfSpace::Bottom,
    OctreeElement::HalfSpace::Top,
    OctreeElement::HalfSpace::Right,
    OctreeElement::HalfSpace::Left,
    OctreeElement::HalfSpace::Near,
    OctreeElement::HalfSpace::Far,
};

// Construction vectors ordered such that the vertices of each face are
// clockwise in a right-handed coordinate system with B-L-N at 0,0,0. 
float Cube::_sVertexIndexToConstructionVector[24][3] = {
    // Bottom
    { 0,0,0 },
    { 1,0,0 },
    { 1,0,1 },
    { 0,0,1 },
    // Top
    { 0,1,0 },
    { 0,1,1 },
    { 1,1,1 },
    { 1,1,0 },
    // Right
    { 1,0,0 },
    { 1,1,0 },
    { 1,1,1 },
    { 1,0,1 },
    // Left
    { 0,0,0 },
    { 0,0,1 },
    { 0,1,1 },
    { 0,1,0 },
    // Near
    { 0,0,0 },
    { 0,1,0 },
    { 1,1,0 },
    { 1,0,0 },
    // Far
    { 0,0,1 },
    { 1,0,1 },
    { 1,1,1 },
    { 0,1,1 },
};

// Normals for a right-handed coordinate system
float Cube::_sVertexIndexToNormalVector[6][3] = {
    {  0,-1, 0 }, // Bottom
    {  0, 1, 0 }, // Top
    {  1, 0, 0 }, // Right
    { -1, 0, 0 }, // Left
    {  0, 0,-1 }, // Near
    {  0, 0, 1 }, // Far
};

Renderer::Renderer() {
}

Renderer::~Renderer() {
}

// Simple dispatch between API and SPI
int Renderer::add(
    Primitive* primitive
    ) {
    return vAdd(primitive);
}

void Renderer::remove(
    int id
    ) {
    vRemove(id);
}

void Renderer::release() {
    vRelease();
}

void Renderer::render() {
    vRender();
}

unsigned long Renderer::getMemoryUsage() {
    return vGetMemoryUsage();
}

unsigned long Renderer::getMemoryUsageGPU() {
    return vGetMemoryUsageGPU();
}

PrimitiveRenderer::PrimitiveRenderer(
    int maxCount
    ) :
        _maxCount(maxCount),
        _triBufferId(0),
        _vertexBufferId(0),

        _vertexElementCount(0),
        _maxVertexElementCount(0),

        _triElementCount(0),
        _maxTriElementCount(0),

        _primitives(),
        _primitiveCount(0),

        _gpuMemoryUsage(0),
        _cpuMemoryUsage(0)

{
    init();
}

PrimitiveRenderer::~PrimitiveRenderer() {

    terminate();
}

void PrimitiveRenderer::init() {

    initializeGL();
    initializeBookkeeping();
}

void PrimitiveRenderer::initializeGL() {

    glGenBuffers(1, &_triBufferId);
    glGenBuffers(1, &_vertexBufferId);

    // Set up the element array buffer containing the index ids
    _maxTriElementCount = _maxCount * 2;
    int size = _maxTriElementCount * _sIndicesPerTri * sizeof(GLint);
    _gpuMemoryUsage += size;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _triBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Set up the array buffer in the form of array of structures
    // I chose AOS because it maximizes the amount of data tranferred
    // by a single glBufferSubData call.
    _maxVertexElementCount = _maxCount * 8;
    size = _maxVertexElementCount * sizeof(VertexElement);
    _gpuMemoryUsage += size;

    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Initialize the first tri element in the buffer to all zeros, the
    // degenerate case
    deconstructTriElement(0);

    // Initialize the first vertex element in the buffer to all zeros, the
    // degenerate case
    deconstructVertexElement(0);
}

void PrimitiveRenderer::initializeBookkeeping() {

    // Start primitive count at one, because zero is reserved for the degenerate triangle
    _primitives.resize(_maxCount + 1);

    // Set the counters
    _primitiveCount = 1;
    _vertexElementCount = 1;
    _triElementCount = 1;

    // Guesstimate the memory consumption
    _cpuMemoryUsage = sizeof(PrimitiveRenderer);
    _cpuMemoryUsage += _availablePrimitiveIndex.capacity() * sizeof(int);
    _cpuMemoryUsage += _availableVertexElementIndex.capacity() * sizeof(int);
    _cpuMemoryUsage += _availableTriElementIndex.capacity() * sizeof(int);
    _cpuMemoryUsage += _deconstructTriElementIndex.capacity() * sizeof(int);
    _cpuMemoryUsage += _constructPrimitiveIndex.capacity() * sizeof(int);
}

void PrimitiveRenderer::terminate() {

    terminateBookkeeping();
    terminateGL();
}

void PrimitiveRenderer::terminateGL() {

    if (_vertexBufferId) {
        glDeleteBuffers(1, &_vertexBufferId);
        _vertexBufferId = 0;
    }

    if (_triBufferId) {
        glDeleteBuffers(1, &_triBufferId);
        _triBufferId = 0;
    }
}

void PrimitiveRenderer::terminateBookkeeping() {

    // Delete all of the primitives
    for (int i = _primitiveCount + 1; --i > 0; ) {
        Primitive* primitive = _primitives[i];
        if (primitive) {
            _cpuMemoryUsage -= primitive->getMemoryUsage();
            _primitives[i] = 0;
            delete primitive;
        }
    }

    // Drain the queues
    _availablePrimitiveIndex.clear();
    _availableVertexElementIndex.clear();
    _availableTriElementIndex.clear();
    _deconstructTriElementIndex.clear();
    _constructPrimitiveIndex.clear();

    _cpuMemoryUsage = sizeof(PrimitiveRenderer) + _primitives.size() * sizeof(Primitive *);
}

void PrimitiveRenderer::constructElements(
    Primitive* primitive
    ) {

    // Load vertex elements
    VertexElementIndexList& vertexElementIndexList = primitive->vertexElementIndices();
    const VertexElementList& vertices = primitive->vertexElements();
    {
        for (VertexElementList::const_iterator it = vertices.begin(); it != vertices.end(); ++it ) {
            int index = getAvailableVertexElementIndex();
            if (index != 0) {
                // Store the vertex element index in the primitive's
                // vertex element index list
                vertexElementIndexList.push_back(index);

                VertexElement* vertex = *it;
                transferVertexElement(index, vertex);
            } else {
                break;
            }
        }
    }

    // Load tri elements
    if (vertexElementIndexList.size() == vertices.size()) {
        TriElementList& tris = primitive->triElements();

        for (TriElementList::iterator it = tris.begin(); it != tris.end(); ++it) {
            TriElement* tri = *it;
            int index = getAvailableTriElementIndex();
            if (index != 0) {
                int k;
                k = tri->indices[0];
                tri->indices[0] = vertexElementIndexList[k];

                k = tri->indices[1];
                tri->indices[1] = vertexElementIndexList[k];

                k = tri->indices[2];
                tri->indices[2] = vertexElementIndexList[k];

                tri->id = index;
                transferTriElement(index, tri->indices);
            } else {
                break;
            }
        }
    } else {
        // TODO: failure mode
    }
}

void PrimitiveRenderer::deconstructElements(
    Primitive* primitive
    ) {

    // Schedule the tri elements of the face for deconstruction
    {
        TriElementList& tris = primitive->triElements();

        for (TriElementList::const_iterator it = tris.begin(); it != tris.end(); ++it) {
            const TriElement* tri = *it;

            if (tri->id) {
                // Put the tri element index into decon queue
                _deconstructTriElementIndex.push(tri->id);
            }
        }
    }

    // Return the vertex element index to the available queue, it is not necessary
    // to zero the data
    {
        VertexElementIndexList& vertexIndexList = primitive->vertexElementIndices();

        for (VertexElementIndexList::const_iterator it = vertexIndexList.begin(); it != vertexIndexList.end(); ++it) {
            int index = *it;

            if (index) {
                // Put the vertex element index into the available queue
                _availableVertexElementIndex.push(index);
            }
        }
    }

    delete primitive;
}

int PrimitiveRenderer::getAvailablePrimitiveIndex() {

    int index;

    // Check the available primitive index queue first for an available index.
    if (!_availablePrimitiveIndex.isEmpty()) {
        index = _availablePrimitiveIndex.pop();
    } else if (_primitiveCount < _maxCount) {
        // There are no primitive indices available from the queue, 
        // make one up
        index = _primitiveCount++;
    } else {
        index = 0;
    }
    return index;
}

int PrimitiveRenderer::getAvailableVertexElementIndex() {

    int index;

    // Check the available vertex element queue first for an available index.
    if (!_availableVertexElementIndex.isEmpty()) {
        index = _availableVertexElementIndex.pop();
    } else if (_vertexElementCount < _maxVertexElementCount) {
        // There are no vertex elements available from the queue, 
        // grab one from the end of the list
        index = _vertexElementCount++;
    } else {
        index = 0;
    }
    return index;
}

int PrimitiveRenderer::getAvailableTriElementIndex() {

    int index;

    // Check the tri elements scheduled for deconstruction queue first to 
    // intercept and reuse an index without it having to be destroyed
    if (!_deconstructTriElementIndex.isEmpty()) {
        index = _deconstructTriElementIndex.pop();
    } else if (!_availableTriElementIndex.isEmpty()) {
        // Nothing available in the deconstruction queue, now
        // check the available tri element queue for an available index.
        index = _availableTriElementIndex.pop();
    } else if (_triElementCount < _maxTriElementCount) {
        // There are no reusable tri elements available from the queue, 
        // grab one from the end of the list
        index = _triElementCount++;
    } else {
        index = 0;
    }
    return index;
}

void PrimitiveRenderer::deconstructTriElement(
    int idx
    ) {

    // Set the tri element to the degenerate case.
    static int degenerate[3] = { 0, 0, 0 };
    transferTriElement(idx, degenerate);

}

void PrimitiveRenderer::deconstructVertexElement(
    int idx
    ) {

    // Set the vertex element to the degenerate case.
    VertexElement degenerate;
    memset(&degenerate, 0, sizeof(degenerate));

    transferVertexElement(idx, &degenerate);

}

void PrimitiveRenderer::transferVertexElement(
    int idx,
    VertexElement* vertex
    ) {

    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, idx * sizeof(VertexElement), sizeof(VertexElement), vertex);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PrimitiveRenderer::transferTriElement(
    int idx,
    int tri[3]
    ) {

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _triBufferId);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, idx * _sBytesPerTriElement, _sBytesPerTriElement, tri);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

int PrimitiveRenderer::vAdd(
    Primitive* primitive
    ) {

    QMutexLocker lock(&_guard);
    int id = getAvailablePrimitiveIndex();
    if (id != 0) {
        // Take ownership of primitive, including responsibility
        // for destruction
        _primitives[id] = primitive;
        _constructPrimitiveIndex.push(id);
        _cpuMemoryUsage += primitive->getMemoryUsage();
    }
    return id;
}

void PrimitiveRenderer::vRemove(
    int id
    ) {

    if (id != 0) {
        QMutexLocker lock(&_guard);

        // Locate and remove the primitive by id in the vector map
        Primitive* primitive = _primitives[id];
        if (primitive) {
            _primitives[id] = 0;
            _cpuMemoryUsage -= primitive->getMemoryUsage();
            deconstructElements(primitive);

            // Queue the index onto the available primitive stack.
            _availablePrimitiveIndex.push(id);
        }
    }
}

void PrimitiveRenderer::vRelease() {

    QMutexLocker lock(&_guard);

    terminateBookkeeping();
    initializeBookkeeping();
}

void PrimitiveRenderer::vRender() {

    int id;
    QMutexLocker lock(&_guard);

    // Iterate over the set of triangle element array buffer ids scheduled for
    // destruction. Set the triangle element to the degenerate case. Queue the id
    // onto the available tri element stack.
    while (!_deconstructTriElementIndex.isEmpty()) {
        id = _deconstructTriElementIndex.pop();
        deconstructTriElement(id);
        _availableTriElementIndex.push(id);
    }

    // Iterate over the set of primitive ids scheduled for construction. Transfer 
    // primitive data to the GPU.
    while (!_constructPrimitiveIndex.isEmpty()) {
        id = _constructPrimitiveIndex.pop();
        Primitive* primitive = _primitives[id];
        if (primitive) {
            constructElements(primitive);

            // No need to keep an extra copy of the vertices
            _cpuMemoryUsage -= primitive->getMemoryUsage();
            primitive->releaseVertexElements();
            _cpuMemoryUsage += primitive->getMemoryUsage();
        }
    }

    // The application uses clockwise winding for the definition of front face, this renderer
    // aalso uses clockwise (that is the gl default) to construct the triangulation
    // so...
    //glFrontFace(GL_CW);
    glEnable(GL_CULL_FACE);

    glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferId);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VertexElement), 0);

    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, sizeof(VertexElement), (const GLvoid*)12);

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexElement), (const GLvoid*)24);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _triBufferId);
    glDrawElements(GL_TRIANGLES, 3 * _triElementCount, GL_UNSIGNED_INT, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisable(GL_CULL_FACE);
}

unsigned long PrimitiveRenderer::vGetMemoryUsage() {
    return _cpuMemoryUsage;
}

unsigned long PrimitiveRenderer::vGetMemoryUsageGPU() {
    return _gpuMemoryUsage;
}