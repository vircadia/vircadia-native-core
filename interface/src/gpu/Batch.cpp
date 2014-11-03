//
//  Batch.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/14/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Batch.h"

#include <QDebug>

#define ADD_COMMAND(call) _commands.push_back(COMMAND_##call); _commandOffsets.push_back(_params.size());

using namespace gpu;

const int Element::TYPE_SIZE[Element::NUM_TYPES] = {
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

const int Element::DIMENSION_COUNT[Element::NUM_DIMENSIONS] = {
    1,
    2,
    3,
    4,
    9,
    16
};

Batch::Batch() :
    _commands(),
    _commandOffsets(),
    _params(),
    _resources(),
    _data(){
}

Batch::~Batch() {
}

void Batch::clear() {
    _commands.clear();
    _commandOffsets.clear();
    _params.clear();
    _resources.clear();
    _buffers.clear();
    _data.clear();
}

uint32 Batch::cacheResource(Resource* res) {
    uint32 offset = _resources.size();
    _resources.push_back(ResourceCache(res));
    
    return offset;
}

uint32 Batch::cacheResource(const void* pointer) {
    uint32 offset = _resources.size();
    _resources.push_back(ResourceCache(pointer));

    return offset;
}

uint32 Batch::cacheData(uint32 size, const void* data) {
    uint32 offset = _data.size();
    uint32 nbBytes = size;
    _data.resize(offset + nbBytes);
    memcpy(_data.data() + offset, data, size);

    return offset;
}

void Batch::draw(Primitive primitiveType, uint32 nbVertices, uint32 startVertex) {
    ADD_COMMAND(draw);

    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
}

void Batch::drawIndexed(Primitive primitiveType, uint32 nbIndices, uint32 startIndex) {
    ADD_COMMAND(drawIndexed);

    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
}

void Batch::drawInstanced(uint32 nbInstances, Primitive primitiveType, uint32 nbVertices, uint32 startVertex, uint32 startInstance) {
    ADD_COMMAND(drawInstanced);

    _params.push_back(startInstance);
    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}

void Batch::drawIndexedInstanced(uint32 nbInstances, Primitive primitiveType, uint32 nbIndices, uint32 startIndex, uint32 startInstance) {
    ADD_COMMAND(drawIndexedInstanced);

    _params.push_back(startInstance);
    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}

void Batch::setInputFormat(const StreamFormatPtr format) {
    ADD_COMMAND(setInputFormat);

    _params.push_back(_streamFormats.cache(format));
}

void Batch::setInputStream(uint8 startChannel, const StreamPtr& stream) {
    if (!stream.isNull()) {
        if (stream->getNumBuffers()) {
            const Buffers& buffers = stream->getBuffers();
            const Offsets& offsets = stream->getOffsets();
            const Offsets& strides = stream->getStrides();
            for (int i = 0; i < buffers.size(); i++) {
                setInputBuffer(startChannel + i, buffers[i], offsets[i], strides[i]);
            }
        }
    }
}

void Batch::setInputBuffer(uint8 channel, const BufferPtr& buffer, Offset offset, Offset stride) {
    ADD_COMMAND(setInputBuffer);

    _params.push_back(stride);
    _params.push_back(offset);
    _params.push_back(_buffers.cache(buffer));
    _params.push_back(channel);
}

void Batch::setIndexBuffer(Element::Type type, const BufferPtr& buffer, Offset offset) {
    ADD_COMMAND(setIndexBuffer);

    _params.push_back(offset);
    _params.push_back(_buffers.cache(buffer));
    _params.push_back(type);
}
