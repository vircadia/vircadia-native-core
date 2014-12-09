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

#include <assert.h>
#include "GPUConfig.h"

#include "Resource.h"
#include "Format.h"
#include <vector>
#include <map>

namespace gpu {

// Stream namespace class
class Stream {
public:

    // Possible input slots identifiers
    enum InputSlot {
        POSITION = 0,
        NORMAL,
        COLOR,
        TEXCOORD,
        TANGENT,
        SKIN_CLUSTER_INDEX,
        SKIN_CLUSTER_WEIGHT,
        TEXCOORD1,

        NUM_INPUT_SLOTS,
    };

    typedef uint8 Slot;

    // Frequency describer
    enum Frequency {
        PER_VERTEX = 0,
        PER_INSTANCE,
    };

    // The attribute description
    // Every thing that is needed to detail a stream attribute and how to interpret it
    class Attribute {
    public:
        Attribute(Slot slot, Slot channel, Element element, Offset offset = 0, Frequency frequency = PER_VERTEX) :
            _slot(slot),
            _channel(channel),
            _element(element),
            _offset(offset),
            _frequency(frequency)
        {}
        Attribute() :
            _slot(POSITION),
            _channel(0),
            _element(),
            _offset(0),
            _frequency(PER_VERTEX)
        {}


        Slot _slot; // Logical slot assigned to the attribute
        Slot _channel; // index of the channel where to get the data from
        Element _element;

        Offset _offset;
        uint32 _frequency;

        // Size of the 
        uint32 getSize() const { return _element.getSize(); }
    };

    // Stream Format is describing how to feed a list of attributes from a bunch of stream buffer channels
    class Format {
    public:
        typedef std::map< Slot, Attribute > AttributeMap;

        class Channel {
        public:
            std::vector< Slot > _slots;
            std::vector< Offset > _offsets;
            Offset _stride;
            uint32 _netSize;

            Channel() : _stride(0), _netSize(0) {}
        };
        typedef std::map< Slot, Channel > ChannelMap;

        Format() :
            _attributes(),
            _elementTotalSize(0) {}
        ~Format() {}

        uint32 getNumAttributes() const { return _attributes.size(); }
        const AttributeMap& getAttributes() const { return _attributes; }

        uint8  getNumChannels() const { return _channels.size(); }
        const ChannelMap& getChannels() const { return _channels; }

        uint32 getElementTotalSize() const { return _elementTotalSize; }

        bool setAttribute(Slot slot, Slot channel, Element element, Offset offset = 0, Frequency frequency = PER_VERTEX);

    protected:
        AttributeMap _attributes;
        ChannelMap _channels;
        uint32 _elementTotalSize;

        void evaluateCache();
    };

    typedef QSharedPointer<Format> FormatPointer;
};

typedef std::vector< Offset > Offsets;

// Buffer Stream is a container of N Buffers and their respective Offsets and Srides representing N consecutive channels.
// A Buffer Stream can be assigned to the Batch to set several stream channels in one call
class BufferStream {
public:
    typedef Offsets Strides;

    BufferStream();
    ~BufferStream();

    void addBuffer(BufferPointer& buffer, Offset offset, Offset stride);

    const Buffers& getBuffers() const { return _buffers; }
    const Offsets& getOffsets() const { return _offsets; }
    const Strides& getStrides() const { return _strides; }
    uint8 getNumBuffers() const { return _buffers.size(); }

protected:
    Buffers _buffers;
    Offsets _offsets;
    Strides _strides;
};
typedef QSharedPointer<BufferStream> BufferStreamPointer;

};


#endif
