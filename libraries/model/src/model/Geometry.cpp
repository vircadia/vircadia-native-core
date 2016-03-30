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

#include <QDebug>

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

