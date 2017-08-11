//
//  Stream.h
//  interface/src/gpu
//
//  Created by Sam Gateau on 10/29/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_gpu_Stream_h
#define hifi_gpu_Stream_h

#include <vector>
#include <map>
#include <array>
#include <string>

#include <assert.h>

#include "Resource.h"
#include "Format.h"

namespace gpu {

// Stream namespace class
class Stream {
public:

    // Possible input slots identifiers
    enum InputSlot {
        POSITION = 0,
        NORMAL = 1,
        COLOR = 2,
        TEXCOORD0 = 3,
        TEXCOORD = TEXCOORD0,
        TANGENT = 4,
        SKIN_CLUSTER_INDEX = 5,
        SKIN_CLUSTER_WEIGHT = 6,
        TEXCOORD1 = 7,
        TEXCOORD2 = 8,
        TEXCOORD3 = 9,
        TEXCOORD4 = 10,

        NUM_INPUT_SLOTS,

        DRAW_CALL_INFO = 15, // Reserve last input slot for draw call infos
    };

    typedef uint8 Slot;

    // Frequency describer
    enum Frequency {
        PER_VERTEX = 0,
        PER_INSTANCE = 1,
    };

    // The attribute description
    // Every thing that is needed to detail a stream attribute and how to interpret it
    class Attribute {
    public:
        Attribute() {}

        Attribute(Slot slot, Slot channel, Element element, Offset offset = 0, Frequency frequency = PER_VERTEX) :
            _slot(slot),
            _channel(channel),
            _element(element),
            _offset(offset),
            _frequency(frequency)
        {}

        Slot _slot{ POSITION }; // Logical slot assigned to the attribute
        Slot _channel{ POSITION }; // index of the channel where to get the data from
        Element _element{ Element::VEC3F_XYZ };
        Offset _offset{ 0 };
        uint32 _frequency{ PER_VERTEX };

        // Size of the 
        uint32 getSize() const { return _element.getSize(); }

        // Generate a string key describing the attribute uniquely
        std::string getKey() const;
    };

    // Stream Format is describing how to feed a list of attributes from a bunch of stream buffer channels
    class Format {
    public:
        typedef std::map< Slot, Attribute > AttributeMap;

        class ChannelInfo {
        public:
            std::vector< Slot > _slots;
            std::vector< Offset > _offsets;
            Offset _stride;
            uint32 _netSize;

            ChannelInfo() : _stride(0), _netSize(0) {}
        };
        typedef std::map< Slot, ChannelInfo > ChannelMap;

        size_t getNumAttributes() const { return _attributes.size(); }
        const AttributeMap& getAttributes() const { return _attributes; }

        size_t getNumChannels() const { return _channels.size(); }
        const ChannelMap& getChannels() const { return _channels; }
        Offset getChannelStride(Slot channel) const { return _channels.at(channel)._stride; }

        size_t getElementTotalSize() const { return _elementTotalSize; }

        bool setAttribute(Slot slot, Slot channel, Element element, Offset offset = 0, Frequency frequency = PER_VERTEX);
        bool setAttribute(Slot slot, Frequency frequency = PER_VERTEX);
        bool setAttribute(Slot slot, Slot channel, Frequency frequency = PER_VERTEX);

        bool hasAttribute(Slot slot) const { return (_attributes.find(slot) != _attributes.end()); }

        const std::string& getKey() const { return _key; }

        const GPUObjectPointer gpuObject{};

    protected:
        AttributeMap _attributes;
        ChannelMap _channels;
        uint32 _elementTotalSize { 0 };
        std::string _key;

        void evaluateCache();
    };

    typedef std::shared_ptr<Format> FormatPointer;
};

typedef std::vector< Offset > Offsets;

// Buffer Stream is a container of N Buffers and their respective Offsets and Srides representing N consecutive channels.
// A Buffer Stream can be assigned to the Batch to set several stream channels in one call
class BufferStream {
public:
    using Strides = Offsets;

    void clear() { _buffers.clear(); _offsets.clear(); _strides.clear(); }
    void addBuffer(const BufferPointer& buffer, Offset offset, Offset stride);

    const Buffers& getBuffers() const { return _buffers; }
    const Offsets& getOffsets() const { return _offsets; }
    const Strides& getStrides() const { return _strides; }
    size_t getNumBuffers() const { return _buffers.size(); }

    BufferStream makeRangedStream(uint32 offset, uint32 count = -1) const;

protected:
    Buffers _buffers;
    Offsets _offsets;
    Strides _strides;
};
typedef std::shared_ptr<BufferStream> BufferStreamPointer;

};


#endif
