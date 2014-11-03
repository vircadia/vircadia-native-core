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
#include "InterfaceConfig.h"

#include "gpu/Resource.h"
#include "gpu/Format.h"
#include <vector>
#include <map>

namespace gpu {

class StreamFormat {
public:

    enum Slot {
        SLOT_POSITION = 0,
        SLOT_NORMAL,
        SLOT_TEXCOORD,
        SLOT_COLOR,
        SLOT_TANGENT,
        SLOT_SKIN_CLUSTER_WEIGHT,
        SLOT_SKIN_CLUSTER_INDEX,

        NUM_SLOTS,
    };

    enum Frequency {
        FREQUENCY_PER_VERTEX = 0,
        FREQUENCY_PER_INSTANCE,
    };

    class Attribute {
    public:
        typedef std::vector< Attribute > vector;

        Attribute(Slot slot, uint8 channel, Element element, Offset offset = 0, Frequency frequency = FREQUENCY_PER_VERTEX) :
            _slot(slot),
            _channel(channel),
            _element(element),
            _offset(offset),
            _frequency(frequency)
        {}
        Attribute() :
            _slot(SLOT_POSITION),
            _channel(0),
            _element(),
            _offset(0),
            _frequency(FREQUENCY_PER_VERTEX)
        {}


        uint8 _slot; // Logical slot assigned to the attribute
        uint8 _channel; // index of the channel where to get the data from
        Element _element;

        Offset _offset;
        uint32 _frequency;

        uint32 getSize() const { return _element.getSize(); }

    };

    typedef std::map< uint8, Attribute > AttributeMap;

    class Channel {
    public:
        std::vector< uint8 > _slots;
        std::vector< uint32 > _offsets;
        uint32 _stride;
        uint32 _netSize;

        Channel() : _stride(0), _netSize(0) {}
    };
    typedef std::map< uint8, Channel > ChannelMap;

    StreamFormat() :
        _attributes(),
        _elementTotalSize(0) {}
    ~StreamFormat() {}

    uint32 getNumAttributes() const { return _attributes.size(); }
    const AttributeMap& getAttributes() const { return _attributes; }

    uint8  getNumChannels() const { return _channels.size(); }
    const ChannelMap& getChannels() const { return _channels; }

    uint32 getElementTotalSize() const { return _elementTotalSize; }

    bool setAttribute(Slot slot, uint8 channel, Element element, Offset offset = 0, Frequency frequency = FREQUENCY_PER_VERTEX);

protected:
    AttributeMap _attributes;
    ChannelMap _channels;
    uint32 _elementTotalSize;

    void evaluateCache();
};

class Stream {
public:
    typedef std::vector< BufferPtr > Buffers;
    typedef std::vector< uint32 > Offsets;
    typedef std::vector< uint32 > Strides;

    Stream();
    ~Stream();

    void addBuffer(BufferPtr& buffer, uint32 offset, uint32 stride);

    const Buffers& getBuffers() const { return _buffers; }
    const Offsets& getOffsets() const { return _offsets; }
    const Strides& getStrides() const { return _strides; }
    uint8 getNumBuffers() const { return _buffers.size(); }

protected:
    Buffers _buffers;
    Offsets _offsets;
    Strides _strides;
};
typedef QSharedPointer<StreamFormat> StreamFormatPtr;
typedef QSharedPointer<Stream> StreamPtr;

};


#endif
