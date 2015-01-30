//
//  BandwidthMeter.cpp
//  interface/src/ui
//
//  Created by Seth Alves on 2015-1-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Based on code by Tobias Schwinger
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <GeometryCache.h>
#include "BandwidthRecorder.h"


BandwidthRecorder::Channel::Channel(char const* const caption,
                                    char const* unitCaption,
                                    double unitScale,
                                    unsigned colorRGBA) :
    caption(caption),
    unitCaption(unitCaption),
    unitScale(unitScale),
    colorRGBA(colorRGBA)
{
}



BandwidthRecorder::BandwidthRecorder() {
}


BandwidthRecorder::~BandwidthRecorder() {
    delete audioChannel;
    delete avatarsChannel;
    delete octreeChannel;
    delete metavoxelsChannel;
    delete totalChannel;
}


BandwidthRecorder::Stream::Stream(float msToAverage) : _value(0.0f), _msToAverage(msToAverage) {
    _prevTime.start();
}


void BandwidthRecorder::Stream::updateValue(double amount) {

    // Determine elapsed time
    double dt = (double)_prevTime.nsecsElapsed() / 1000000.0; // ns to ms

    // Ignore this value when timer imprecision yields dt = 0
    if (dt == 0.0) {
        return;
    }

    _prevTime.start();

    // Compute approximate average
    _value = glm::mix(_value, amount / dt,
                      glm::clamp(dt / _msToAverage, 0.0, 1.0));
}
