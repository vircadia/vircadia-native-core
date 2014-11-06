//
//  Stream.cpp
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/29/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Stream.h"

using namespace gpu;

void Stream::Format::evaluateCache() {
    _channels.clear();
    _elementTotalSize = 0;
    for(AttributeMap::iterator it = _attributes.begin(); it != _attributes.end(); it++) {
        Attribute& attrib = (*it).second;
        Channel& channel = _channels[attrib._channel];
        channel._slots.push_back(attrib._slot);
        channel._stride = std::max(channel._stride, attrib.getSize() + attrib._offset);
        channel._netSize += attrib.getSize();
        _elementTotalSize += attrib.getSize();
    }
}

bool Stream::Format::setAttribute(Slot slot, Slot channel, Element element, Offset offset, Frequency frequency) {
    _attributes[slot] = Attribute((InputSlot) slot, channel, element, offset, frequency);
    evaluateCache();
    return true;
}

BufferStream::BufferStream() :
    _buffers(),
    _offsets(),
    _strides()
{}

BufferStream::~BufferStream() {
}

void BufferStream::addBuffer(BufferPointer& buffer, Offset offset, Offset stride) {
    _buffers.push_back(buffer);
    _offsets.push_back(offset);
    _strides.push_back(stride);
}
