//
//  BandwidthMeter.h
//  interface
//
//  Created by Tobias Schwinger on 6/20/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__BandwidthMeter__
#define __interface__BandwidthMeter__

#include <glm/glm.hpp>

#include "ui/TextRenderer.h"


class BandwidthMeter {

public:

    BandwidthMeter();

    void render(int x, int y, unsigned w, unsigned h);

    // Number of channels / streams.
    static size_t const N_CHANNELS = 3;
    static size_t const N_STREAMS = N_CHANNELS * 2;

    // Meta information held for a communication channel (bidirectional).
    struct ChannelInfo {

        char const* caption;
        char const* unitCaption;
        double      unitScale;
        double      unitsMax;
        unsigned    colorRGBA;
    };

    // Representation of a data stream (unidirectional; input or output).
    class Stream {

    public:

        Stream(float secondsToAverage = 3.0f);
        void updateValue(double amount);
        double getValue()                               const   { return _value; }

    private:
        double  _value;                 // Current value.
        timeval _prevTime;              // Time of last feed.
        float   _secsToAverage;         // Seconds to average.
    };

    // Data model accessors
    Stream& inputStream(unsigned channelIndex)                  { return _streams[channelIndex * 2]; }
    Stream const& inputStream(unsigned channelIndex)    const   { return _streams[channelIndex * 2]; }
    Stream& outputStream(unsigned channelIndex)                 { return _streams[channelIndex * 2 + 1]; }
    Stream const& outputStream(unsigned channelIndex)   const   { return _streams[channelIndex * 2 + 1]; }
    ChannelInfo& channelInfo(unsigned index)                    { return _channels[index]; }
    ChannelInfo const& channelInfo(unsigned index)      const   { return _channels[index]; }

private:
    TextRenderer _textRenderer;
    ChannelInfo _channels[N_CHANNELS];
    Stream _streams[N_STREAMS];

    static ChannelInfo _DEFAULT_CHANNELS[];
    static Stream _DEFAULT_STREAMS[];
};

#endif /* defined(__interface__BandwidthMeter__) */

