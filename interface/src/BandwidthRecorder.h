//
//  BandwidthRecorder.h
//
//  Created by Seth Alves on 2015-1-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Based on code by Tobias Schwinger
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_BandwidthRecorder_h
#define hifi_BandwidthRecorder_h

#include <QElapsedTimer>

const double DEFAULT_UNIT_SCALE = 8000.0 / 1024.0;

const unsigned int COLOR0 = 0x33cc99ff;
const unsigned int COLOR1 = 0xffef40c0;
const unsigned int COLOR2 = 0xd0d0d0a0;


class BandwidthRecorder {
 public:
    BandwidthRecorder();
    ~BandwidthRecorder();

    // keep track of data rate in one direction
    class Stream {
    public:
        Stream(float msToAverage = 3000.0f);
        void updateValue(double amount);
        double getValue() const { return _value; }
    private:
        double _value; // Current value.
        double _msToAverage; // Milliseconds to average.
        QElapsedTimer _prevTime; // Time of last feed.
    };

    // keep track of data rate in two directions as well as units and style to use during display
    class Channel {
    public:
        Channel(char const* const caption, char const* unitCaption, double unitScale, unsigned colorRGBA);
        Stream input;
        Stream output;
        char const* const caption;
        char const* unitCaption;
        double unitScale;
        unsigned colorRGBA;
    };

    // create the channels we keep track of
    Channel* audioChannel = new Channel("Audio", "Kbps", DEFAULT_UNIT_SCALE, COLOR0);
    Channel* avatarsChannel = new Channel("Avatars", "Kbps", DEFAULT_UNIT_SCALE, COLOR1);
    Channel* octreeChannel = new Channel("Octree", "Kbps", DEFAULT_UNIT_SCALE, COLOR2);
    Channel* metavoxelsChannel = new Channel("Metavoxels", "Kbps", DEFAULT_UNIT_SCALE, COLOR2);
    Channel* totalChannel = new Channel("Total", "Kbps", DEFAULT_UNIT_SCALE, COLOR2);
};

#endif
