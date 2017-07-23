//
//  Geometry.cpp
//  libraries/model/src/model
//
//  Created by Sam Gateau on 12/5/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Geometry.h"

using namespace model;

Mesh::Mesh() :
    _vertexBuffer(gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ)),
    _indexBuffer(gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::INDEX)),
    _partBuffer(gpu::Element(gpu::VEC4, gpu::UINT32, gpu::PART)) {
}

Mesh::Mesh(const Mesh& mesh) :
    _vertexFormat(mesh._vertexFormat),
    _vertexBuffer(mesh._vertexBuffer),
    _attributeBuffers(mesh._attributeBuffers),
    _indexBuffer(mesh._indexBuffer),
    _partBuffer(mesh._partBuffer) {
}

Mesh::~Mesh() {
}

void Mesh::setVertexBuffer(const BufferView& buffer) {
    _vertexBuffer = buffer;
    evalVertexFormat();
}

void Mesh::addAttribute(Slot slot, const BufferView& buffer) {
    _attributeBuffers[slot] = buffer;
    evalVertexFormat();
}

const BufferView Mesh::getAttributeBuffer(int attrib) const {
    auto attribBuffer = _attributeBuffers.find(attrib);
    if (attribBuffer != _attributeBuffers.end()) {
        return attribBuffer->second;
    } else {
        return BufferView();
    }
}

void Mesh::evalVertexFormat() {
    auto vf = new VertexFormat();
    int channelNum = 0;
    if (hasVertexData()) {
        vf->setAttribute(gpu::Stream::POSITION, channelNum, _vertexBuffer._element, 0);
        channelNum++;
    }
    for (auto attrib : _attributeBuffers) {
        vf->setAttribute(attrib.first, channelNum, attrib.second._element, 0);
        channelNum++;
    }

    _vertexFormat.reset(vf);

    evalVertexStream();
}


void Mesh::evalVertexStream() {
    _vertexStream.clear();

    int channelNum = 0;
    if (hasVertexData()) {
        _vertexStream.addBuffer(_vertexBuffer._buffer, _vertexBuffer._offset, _vertexFormat->getChannelStride(channelNum));
        channelNum++;
    }
    for (auto attrib : _attributeBuffers) {
        BufferView& view = attrib.second;
        _vertexStream.addBuffer(view._buffer, view._offset, _vertexFormat->getChannelStride(channelNum));
        channelNum++;
    }
}

void Mesh::setIndexBuffer(const BufferView& buffer) {
    _indexBuffer = buffer;
}

void Mesh::setPartBuffer(const BufferView& buffer) {
    _partBuffer = buffer;
}

Box Mesh::evalPartBound(int partNum) const {
    Box box;
    if (partNum < _partBuffer.getNum<Part>()) {
        const Part& part = _partBuffer.get<Part>(partNum);
        auto index = _indexBuffer.cbegin<Index>();
        index += part._startIndex;
        auto endIndex = index;
        endIndex += part._numIndices;
        auto vertices = &_vertexBuffer.get<Vec3>(part._baseVertex);
        for (;index != endIndex; index++) {
            // skip primitive restart indices
            if ((*index) != PRIMITIVE_RESTART_INDEX) {
                box += vertices[(*index)];
            }
        }
    }
    return box;
}

Box Mesh::evalPartsBound(int partStart, int partEnd) const {
    Box totalBound;
    auto part = _partBuffer.cbegin<Part>() + partStart;
    auto partItEnd = _partBuffer.cbegin<Part>() + partEnd;

    for (;part != partItEnd; part++) {

        Box partBound;
        auto index = _indexBuffer.cbegin<uint>() + (*part)._startIndex;
        auto endIndex = index + (*part)._numIndices;
        auto vertices = &_vertexBuffer.get<Vec3>((*part)._baseVertex);
        for (;index != endIndex; index++) {
            // skip primitive restart indices
            if ((*index) != (uint) PRIMITIVE_RESTART_INDEX) {
                partBound += vertices[(*index)];
            }
        }

        totalBound += partBound;
    }
    return totalBound;
}


model::MeshPointer Mesh::map(std::function<glm::vec3(glm::vec3)> vertexFunc,
                             std::function<glm::vec3(glm::vec3)> colorFunc,
                             std::function<glm::vec3(glm::vec3)> normalFunc,
                             std::function<uint32_t(uint32_t)> indexFunc) const {
    // vertex data
    const gpu::BufferView& vertexBufferView = getVertexBuffer();
    gpu::BufferView::Index numVertices = (gpu::BufferView::Index)getNumVertices();

    gpu::Resource::Size vertexSize = numVertices * sizeof(glm::vec3);
    unsigned char* resultVertexData = new unsigned char[vertexSize];
    unsigned char* vertexDataCursor = resultVertexData;

    for (gpu::BufferView::Index i = 0; i < numVertices; i++) {
        glm::vec3 pos = vertexFunc(vertexBufferView.get<glm::vec3>(i));
        memcpy(vertexDataCursor, &pos, sizeof(pos));
        vertexDataCursor += sizeof(pos);
    }

    // color data
    int attributeTypeColor = gpu::Stream::COLOR;
    const gpu::BufferView& colorsBufferView = getAttributeBuffer(attributeTypeColor);
    gpu::BufferView::Index numColors = (gpu::BufferView::Index)colorsBufferView.getNumElements();

    gpu::Resource::Size colorSize = numColors * sizeof(glm::vec3);
    unsigned char* resultColorData = new unsigned char[colorSize];
    unsigned char* colorDataCursor = resultColorData;

    for (gpu::BufferView::Index i = 0; i < numColors; i++) {
        glm::vec3 color = colorFunc(colorsBufferView.get<glm::vec3>(i));
        memcpy(colorDataCursor, &color, sizeof(color));
        colorDataCursor += sizeof(color);
    }

    // normal data
    int attributeTypeNormal = gpu::Stream::InputSlot::NORMAL; // libraries/gpu/src/gpu/Stream.h
    const gpu::BufferView& normalsBufferView = getAttributeBuffer(attributeTypeNormal);
    gpu::BufferView::Index numNormals = (gpu::BufferView::Index)normalsBufferView.getNumElements();
    gpu::Resource::Size normalSize = numNormals * sizeof(glm::vec3);
    unsigned char* resultNormalData = new unsigned char[normalSize];
    unsigned char* normalDataCursor = resultNormalData;

    for (gpu::BufferView::Index i = 0; i < numNormals; i++) {
        glm::vec3 normal = normalFunc(normalsBufferView.get<glm::vec3>(i));
        memcpy(normalDataCursor, &normal, sizeof(normal));
        normalDataCursor += sizeof(normal);
    }
    // TODO -- other attributes

    // face data
    const gpu::BufferView& indexBufferView = getIndexBuffer();
    gpu::BufferView::Index numIndexes = (gpu::BufferView::Index)getNumIndices();
    gpu::Resource::Size indexSize = numIndexes * sizeof(uint32_t);
    unsigned char* resultIndexData = new unsigned char[indexSize];
    unsigned char* indexDataCursor = resultIndexData;

    for (gpu::BufferView::Index i = 0; i < numIndexes; i++) {
        uint32_t index = indexFunc(indexBufferView.get<uint32_t>(i));
        memcpy(indexDataCursor, &index, sizeof(index));
        indexDataCursor += sizeof(index);
    }

    model::MeshPointer result(new model::Mesh());

    gpu::Element vertexElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
    gpu::Buffer* resultVertexBuffer = new gpu::Buffer(vertexSize, resultVertexData);
    gpu::BufferPointer resultVertexBufferPointer(resultVertexBuffer);
    gpu::BufferView resultVertexBufferView(resultVertexBufferPointer, vertexElement);
    result->setVertexBuffer(resultVertexBufferView);

    gpu::Element colorElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
    gpu::Buffer* resultColorsBuffer = new gpu::Buffer(colorSize, resultColorData);
    gpu::BufferPointer resultColorsBufferPointer(resultColorsBuffer);
    gpu::BufferView resultColorsBufferView(resultColorsBufferPointer, colorElement);
    result->addAttribute(attributeTypeColor, resultColorsBufferView);

    gpu::Element normalElement = gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::XYZ);
    gpu::Buffer* resultNormalsBuffer = new gpu::Buffer(normalSize, resultNormalData);
    gpu::BufferPointer resultNormalsBufferPointer(resultNormalsBuffer);
    gpu::BufferView resultNormalsBufferView(resultNormalsBufferPointer, normalElement);
    result->addAttribute(attributeTypeNormal, resultNormalsBufferView);

    gpu::Element indexElement = gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW);
    gpu::Buffer* resultIndexesBuffer = new gpu::Buffer(indexSize, resultIndexData);
    gpu::BufferPointer resultIndexesBufferPointer(resultIndexesBuffer);
    gpu::BufferView resultIndexesBufferView(resultIndexesBufferPointer, indexElement);
    result->setIndexBuffer(resultIndexesBufferView);


    // TODO -- shouldn't assume just one part

    std::vector<model::Mesh::Part> parts;
    parts.emplace_back(model::Mesh::Part((model::Index)0, // startIndex
                                         (model::Index)result->getNumIndices(), // numIndices
                                         (model::Index)0, // baseVertex
                                         model::Mesh::TRIANGLES)); // topology
    result->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(model::Mesh::Part),
                                                          (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));

    return result;
}


void Mesh::forEach(std::function<void(glm::vec3)> vertexFunc,
                   std::function<void(glm::vec3)> colorFunc,
                   std::function<void(glm::vec3)> normalFunc,
                   std::function<void(uint32_t)> indexFunc) {
    // vertex data
    const gpu::BufferView& vertexBufferView = getVertexBuffer();
    gpu::BufferView::Index numVertices = (gpu::BufferView::Index)getNumVertices();
    for (gpu::BufferView::Index i = 0; i < numVertices; i++) {
        vertexFunc(vertexBufferView.get<glm::vec3>(i));
    }

    // color data
    int attributeTypeColor = gpu::Stream::InputSlot::COLOR; // libraries/gpu/src/gpu/Stream.h
    const gpu::BufferView& colorsBufferView = getAttributeBuffer(attributeTypeColor);
    gpu::BufferView::Index numColors =  (gpu::BufferView::Index)colorsBufferView.getNumElements();
    for (gpu::BufferView::Index i = 0; i < numColors; i++) {
        colorFunc(colorsBufferView.get<glm::vec3>(i));
    }

    // normal data
    int attributeTypeNormal = gpu::Stream::InputSlot::NORMAL; // libraries/gpu/src/gpu/Stream.h
    const gpu::BufferView& normalsBufferView = getAttributeBuffer(attributeTypeNormal);
    gpu::BufferView::Index numNormals =  (gpu::BufferView::Index)normalsBufferView.getNumElements();
    for (gpu::BufferView::Index i = 0; i < numNormals; i++) {
        normalFunc(normalsBufferView.get<glm::vec3>(i));
    }
    // TODO -- other attributes

    // face data
    const gpu::BufferView& indexBufferView = getIndexBuffer();
    gpu::BufferView::Index numIndexes = (gpu::BufferView::Index)getNumIndices();
    for (gpu::BufferView::Index i = 0; i < numIndexes; i++) {
        indexFunc(indexBufferView.get<uint32_t>(i));
    }
}

MeshPointer Mesh::createIndexedTriangles_P3F(uint32_t numVertices, uint32_t numIndices, const glm::vec3* vertices, const uint32_t* indices) {
    MeshPointer mesh;
    if (numVertices == 0) { return mesh; }
    if (numIndices < 3) { return mesh; }

    mesh = std::make_shared<Mesh>();

    // Vertex buffer
    mesh->setVertexBuffer(gpu::BufferView(new gpu::Buffer(numVertices * sizeof(glm::vec3), (gpu::Byte*) vertices), gpu::Element::VEC3F_XYZ));

    // trim down the indices to shorts if possible
    if (numIndices < std::numeric_limits<uint16_t>::max()) {
        Indices16 shortIndicesVector;
        int16_t* shortIndices = nullptr;
        if (indices) {
            shortIndicesVector.resize(numIndices);
            for (uint32_t i = 0; i < numIndices; i++) {
                shortIndicesVector[i] = indices[i];
            }
            shortIndices = shortIndicesVector.data();
        }

        mesh->setIndexBuffer(gpu::BufferView(new gpu::Buffer(numIndices * sizeof(uint16_t), (gpu::Byte*) shortIndices), gpu::Element::INDEX_UINT16));
    } else {

        mesh->setIndexBuffer(gpu::BufferView(new gpu::Buffer(numIndices * sizeof(uint32_t), (gpu::Byte*) indices), gpu::Element::INDEX_INT32));
    }

    
    std::vector<model::Mesh::Part> parts;
    parts.push_back(model::Mesh::Part(0, numIndices, 0, model::Mesh::TRIANGLES));
    mesh->setPartBuffer(gpu::BufferView(new gpu::Buffer(parts.size() * sizeof(model::Mesh::Part), (gpu::Byte*) parts.data()), gpu::Element::PART_DRAWCALL));

    return mesh;
}


Geometry::Geometry() {
}

Geometry::Geometry(const Geometry& geometry):
    _mesh(geometry._mesh),
    _boxes(geometry._boxes) {
}

Geometry::~Geometry() {
}

void Geometry::setMesh(const MeshPointer& mesh) {
    _mesh = mesh;
}


