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

//#define ADD_COMMAND(call) _commands.push_back(COMMAND_##call); _commandCalls.push_back(&gpu::Batch::do_##call); _commandOffsets.push_back(_params.size());
#define ADD_COMMAND(call) _commands.push_back(COMMAND_##call); _commandOffsets.push_back(_params.size());

using namespace gpu;

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

void Batch::draw(Primitive primitiveType, int nbVertices, int startVertex) {
    ADD_COMMAND(draw);

    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
}

void Batch::drawIndexed(Primitive primitiveType, int nbIndices, int startIndex) {
    ADD_COMMAND(drawIndexed);

    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
}

void Batch::drawInstanced(uint32 nbInstances, Primitive primitiveType, int nbVertices, int startVertex, int startInstance) {
    ADD_COMMAND(drawInstanced);

    _params.push_back(startInstance);
    _params.push_back(startVertex);
    _params.push_back(nbVertices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}

void Batch::drawIndexedInstanced(uint32 nbInstances, Primitive primitiveType, int nbIndices, int startIndex, int startInstance) {
    ADD_COMMAND(drawIndexedInstanced);

    _params.push_back(startInstance);
    _params.push_back(startIndex);
    _params.push_back(nbIndices);
    _params.push_back(primitiveType);
    _params.push_back(nbInstances);
}



