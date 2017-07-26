//
//  Geometry.h
//  libraries/model/src/model
//
//  Created by Sam Gateau on 12/5/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_model_Geometry_h
#define hifi_model_Geometry_h

#include <glm/glm.hpp>

#include <AABox.h>

#include <gpu/Resource.h>
#include <gpu/Stream.h>

namespace model {
typedef gpu::BufferView::Index Index;
typedef gpu::BufferView BufferView;
typedef AABox Box;
typedef std::vector< Box > Boxes;
typedef glm::vec3 Vec3;

class Mesh;
using MeshPointer = std::shared_ptr< Mesh >;


class Mesh {
public:
    const static Index PRIMITIVE_RESTART_INDEX = -1;

    typedef gpu::BufferView BufferView;
    typedef std::vector< BufferView > BufferViews;

    typedef gpu::Stream::Slot Slot;
    typedef gpu::Stream::Format VertexFormat;
    typedef std::map< Slot, BufferView > BufferViewMap;

    typedef model::Vec3 Vec3;

    Mesh();
    Mesh(const Mesh& mesh);
    Mesh& operator= (const Mesh& mesh) = default;
    virtual ~Mesh();

    // Vertex buffer
    void setVertexBuffer(const BufferView& buffer);
    const BufferView& getVertexBuffer() const { return _vertexBuffer; }
    size_t getNumVertices() const { return _vertexBuffer.getNumElements(); }
    bool hasVertexData() const { return _vertexBuffer._buffer.get() != nullptr; }

    // Attribute Buffers
    size_t getNumAttributes() const { return _attributeBuffers.size(); }
    void addAttribute(Slot slot, const BufferView& buffer);
    const BufferView getAttributeBuffer(int attrib) const;

    // Stream format
    const gpu::Stream::FormatPointer getVertexFormat() const { return _vertexFormat; }

    // BufferStream on the mesh vertices and attributes matching the vertex format
    const gpu::BufferStream& getVertexStream() const { return _vertexStream; }

    // Index Buffer
    using Indices16 = std::vector<int16_t>;
    using Indices32 = std::vector<int32_t>;

    void setIndexBuffer(const BufferView& buffer);
    const BufferView& getIndexBuffer() const { return _indexBuffer; }
    size_t getNumIndices() const { return _indexBuffer.getNumElements(); }

    // Access vertex position value
    const Vec3& getPos3(Index index) const { return _vertexBuffer.get<Vec3>(index); }

    enum Topology {
        POINTS = 0,
        LINES,
        LINE_STRIP,
        TRIANGLES,
        TRIANGLE_STRIP,
        QUADS,  // NOTE: These must be translated to triangles before rendering
        QUAD_STRIP,

        NUM_TOPOLOGIES,
    };

    // Subpart of a mesh, describing the toplogy of the surface
    class Part {
    public:
        Index _startIndex;
        Index _numIndices;
        Index _baseVertex;
        Topology _topology;

        Part() :
            _startIndex(0),
            _numIndices(0),
            _baseVertex(0),
            _topology(TRIANGLES)
            {}
        Part(Index startIndex, Index numIndices, Index baseVertex, Topology topology) :
            _startIndex(startIndex),
            _numIndices(numIndices),
            _baseVertex(baseVertex),
            _topology(topology)
            {}
    };

    void setPartBuffer(const BufferView& buffer);
    const BufferView& getPartBuffer() const { return _partBuffer; }
    size_t getNumParts() const { return _partBuffer.getNumElements(); }

    // evaluate the bounding box of A part
    Box evalPartBound(int partNum) const;
    // evaluate the bounding boxes of the parts in the range [start, end]
    // the returned box is the bounding box of ALL the evaluated parts bound.
    Box evalPartsBound(int partStart, int partEnd) const;

    static gpu::Primitive topologyToPrimitive(Topology topo) { return static_cast<gpu::Primitive>(topo); }

    // create a copy of this mesh after passing its vertices, normals, and indexes though the provided functions
    MeshPointer map(std::function<glm::vec3(glm::vec3)> vertexFunc,
                    std::function<glm::vec3(glm::vec3)> colorFunc,
                    std::function<glm::vec3(glm::vec3)> normalFunc,
                    std::function<uint32_t(uint32_t)> indexFunc) const;

    void forEach(std::function<void(glm::vec3)> vertexFunc,
                 std::function<void(glm::vec3)> colorFunc,
                 std::function<void(glm::vec3)> normalFunc,
                 std::function<void(uint32_t)> indexFunc);


    static MeshPointer createIndexedTriangles_P3F(uint32_t numVertices, uint32_t numTriangles, const glm::vec3* vertices = nullptr, const uint32_t* indices = nullptr);

protected:

    gpu::Stream::FormatPointer _vertexFormat;
    gpu::BufferStream _vertexStream;

    BufferView _vertexBuffer;
    BufferViewMap _attributeBuffers;

    BufferView _indexBuffer;

    BufferView _partBuffer;

    void evalVertexFormat();
    void evalVertexStream();

};


class Geometry {
public:

    Geometry();
    Geometry(const Geometry& geometry);
    ~Geometry();

    void setMesh(const MeshPointer& mesh);
    const MeshPointer& getMesh() const { return _mesh; }

protected:

    MeshPointer _mesh;
    BufferView _boxes;

};

};

#endif
