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

#include <algorithm> //min max and more
#include <sstream>
#include <iomanip>

using namespace gpu;

using ElementArray = std::array<Element, Stream::NUM_INPUT_SLOTS>;

const ElementArray& getDefaultElements() {
    static ElementArray defaultElements{{
        //POSITION = 0,
        Element::VEC3F_XYZ,
        //NORMAL = 1,
        Element::VEC3F_XYZ,
        //COLOR = 2,
        Element::COLOR_RGBA_32,
        //TEXCOORD0 = 3,
        Element::VEC2F_UV,
        //TANGENT = 4,
        Element::VEC3F_XYZ,
        //SKIN_CLUSTER_INDEX = 5,
        Element::VEC4F_XYZW,
        //SKIN_CLUSTER_WEIGHT = 6,
        Element::VEC4F_XYZW,
        //TEXCOORD1 = 7,
        Element::VEC2F_UV,
        //TEXCOORD2 = 7,
        Element::VEC4F_XYZW,
        //TEXCOORD3 = 7,
        Element::VEC4F_XYZW,
        //TEXCOORD4 = 7,
        Element::VEC4F_XYZW
        }};
    return defaultElements;
}

std::string Stream::Attribute::getKey() const {
    std::stringstream skey;

    skey << std::hex;
    skey << std::setw(8) << std::setfill('0') << (uint32)((((uint32)_slot) << 24) | (((uint32)_channel) << 16) | ((uint32)_element.getRaw()));
    skey << _offset;
    skey << _frequency;
    return skey.str();
}

void Stream::Format::evaluateCache() {
    _key.clear();
    _channels.clear();
    _elementTotalSize = 0;

    for(AttributeMap::iterator it = _attributes.begin(); it != _attributes.end(); it++) {
        Attribute& attrib = (*it).second;
        ChannelInfo& channel = _channels[attrib._channel];
        channel._slots.push_back(attrib._slot);
        channel._stride = std::max(channel._stride, attrib.getSize() + attrib._offset);
        channel._netSize += attrib.getSize();
        _elementTotalSize += attrib.getSize();

        _key += attrib.getKey();
    }
}

bool Stream::Format::setAttribute(Slot slot, Slot channel, Element element, Offset offset, Frequency frequency) {
    _attributes[slot] = Attribute((InputSlot) slot, channel, element, offset, frequency);
    evaluateCache();
    return true;
}

bool Stream::Format::setAttribute(Slot slot, Frequency frequency) {
    _attributes[slot] = Attribute((InputSlot)slot, slot, getDefaultElements()[slot], 0, frequency);
    evaluateCache();
    return true;
}

bool Stream::Format::setAttribute(Slot slot, Slot channel, Frequency frequency) {
    _attributes[slot] = Attribute((InputSlot)slot, channel, getDefaultElements()[slot], 0, frequency);
    evaluateCache();
    return true;
}

void BufferStream::addBuffer(const BufferPointer& buffer, Offset offset, Offset stride) {
    _buffers.push_back(buffer);
    _offsets.push_back(offset);
    _strides.push_back(stride);
}

BufferStream BufferStream::makeRangedStream(uint32 offset, uint32 count) const {
    if ((offset < _buffers.size())) {
        auto rangeSize = std::min(count, (uint32)(_buffers.size() - offset));
        BufferStream newStream;
        newStream._buffers.insert(newStream._buffers.begin(), _buffers.begin() + offset, _buffers.begin() + offset + rangeSize);
        newStream._offsets.insert(newStream._offsets.begin(), _offsets.begin() + offset, _offsets.begin() + offset + rangeSize);
        newStream._strides.insert(newStream._strides.begin(), _strides.begin() + offset, _strides.begin() + offset + rangeSize);
        return newStream;
    }

    return BufferStream();
}
