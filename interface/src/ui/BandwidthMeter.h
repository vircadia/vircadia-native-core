//
//  BandwidthMeter.h
//  interface/src/ui
//
//  Created by Tobias Schwinger on 6/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_BandwidthMeter_h
#define hifi_BandwidthMeter_h

#include <QElapsedTimer>

#include <glm/glm.hpp>

#include "ui/TextRenderer.h"


class BandwidthMeter {

public:

    BandwidthMeter();
    ~BandwidthMeter();

    void render(int screenWidth, int screenHeight);
    bool isWithinArea(int x, int y, int screenWidth, int screenHeight);

    // Number of channels / streams.
    static size_t const N_CHANNELS = 4;
    static size_t const N_STREAMS = N_CHANNELS * 2;

    // Channel usage.
    enum ChannelIndex { AUDIO, AVATARS, VOXELS, METAVOXELS };

    // Meta information held for a communication channel (bidirectional).
    struct ChannelInfo {

        char const* const   caption;
        char const*         unitCaption;
        double              unitScale;
        unsigned            colorRGBA;
    };

    // Representation of a data stream (unidirectional; input or output).
    class Stream {

    public:

        Stream(float msToAverage = 3000.0f);
        void updateValue(double amount);
        double getValue()                               const   { return _value; }

    private:
        double  _value;                 // Current value.
        double  _msToAverage;           // Milliseconds to average.
        QElapsedTimer _prevTime;              // Time of last feed.
    };

    // Data model accessors
    Stream& inputStream(ChannelIndex i)                         { return _streams[i * 2]; }
    Stream const& inputStream(ChannelIndex i)           const   { return _streams[i * 2]; }
    Stream& outputStream(ChannelIndex i)                        { return _streams[i * 2 + 1]; }
    Stream const& outputStream(ChannelIndex i)          const   { return _streams[i * 2 + 1]; }
    ChannelInfo& channelInfo(ChannelIndex i)                    { return _channels[i]; }
    ChannelInfo const& channelInfo(ChannelIndex i)      const   { return _channels[i]; }

private:
    static void setColorRGBA(unsigned c);
    static void renderBox(int x, int y, int w, int h);
    static void renderVerticalLine(int x, int y, int h);

    static inline int centered(int subject, int object);


    static ChannelInfo _CHANNELS[];

    TextRenderer* _textRenderer;
    ChannelInfo* _channels;
    Stream _streams[N_STREAMS];
    int _scaleMaxIndex;
};

#endif // hifi_BandwidthMeter_h
