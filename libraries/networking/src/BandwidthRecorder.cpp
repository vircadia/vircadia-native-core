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

#include "BandwidthRecorder.h"

#include <QDateTime>

BandwidthRecorder::Channel::Channel() {
}

float BandwidthRecorder::Channel::getAverageInputPacketsPerSecond() const {
    float averageTimeBetweenPackets = _input.getEventDeltaAverage();
    if (averageTimeBetweenPackets > 0.0f) {
        return (1.0f / averageTimeBetweenPackets);
    }
    return 0.0f;
}

float BandwidthRecorder::Channel::getAverageOutputPacketsPerSecond() const {
    float averageTimeBetweenPackets = _output.getEventDeltaAverage();
    if (averageTimeBetweenPackets > 0.0f) {
        return (1.0f / averageTimeBetweenPackets);
    }
    return 0.0f;
}

float BandwidthRecorder::Channel::getAverageInputKilobitsPerSecond() const {
    return (_input.getAverageSampleValuePerSecond() * (8.0f / 1000));
}

float BandwidthRecorder::Channel::getAverageOutputKilobitsPerSecond() const {
    return (_output.getAverageSampleValuePerSecond() * (8.0f / 1000));
}


void BandwidthRecorder::Channel::updateInputAverage(const float sample) {
    _input.updateAverage(sample);
}

void BandwidthRecorder::Channel::updateOutputAverage(const float sample) {
    _output.updateAverage(sample);
}

BandwidthRecorder::BandwidthRecorder() {
    for (uint i=0; i<CHANNEL_COUNT; i++) {
        _channels[ i ] = NULL;
    }
}

BandwidthRecorder::~BandwidthRecorder() {
    for (uint i=0; i<CHANNEL_COUNT; i++) {
        delete _channels[ i ];
    }
}

void BandwidthRecorder::updateInboundData(const quint8 channelType, const int sample) {
    if (! _channels[channelType]) {
        _channels[channelType] = new Channel();
    }
    _channels[channelType]->updateInputAverage(sample);
}

void BandwidthRecorder::updateOutboundData(const quint8 channelType, const int sample) {
    if (! _channels[channelType]) {
        _channels[channelType] = new Channel();
    }
    _channels[channelType]->updateOutputAverage(sample);
}

float BandwidthRecorder::getAverageInputPacketsPerSecond(const quint8 channelType) const {
    if (! _channels[channelType]) {
        return 0.0f;
    }
    return _channels[channelType]->getAverageInputPacketsPerSecond();
}

float BandwidthRecorder::getAverageOutputPacketsPerSecond(const quint8 channelType) const {
    if (! _channels[channelType]) {
        return 0.0f;
    }
    return _channels[channelType]->getAverageOutputPacketsPerSecond();
}

float BandwidthRecorder::getAverageInputKilobitsPerSecond(const quint8 channelType) const {
    if (! _channels[channelType]) {
        return 0.0f;
    }
    return _channels[channelType]->getAverageInputKilobitsPerSecond();
}

float BandwidthRecorder::getAverageOutputKilobitsPerSecond(const quint8 channelType) const {
    if (! _channels[channelType]) {
        return 0.0f;
    }
    return _channels[channelType]->getAverageOutputKilobitsPerSecond();
}

float BandwidthRecorder::getTotalAverageInputPacketsPerSecond() const {
    float result = 0.0f;
    for (uint i=0; i<CHANNEL_COUNT; i++) {
        if (_channels[i]) {
            result += _channels[i]->getAverageInputPacketsPerSecond();
        }
    }
    return result;
}

float BandwidthRecorder::getTotalAverageOutputPacketsPerSecond() const {
    float result = 0.0f;
    for (uint i=0; i<CHANNEL_COUNT; i++) {
        if (_channels[i]) {
            result += _channels[i]->getAverageOutputPacketsPerSecond();
        }
    }
    return result;
}

float BandwidthRecorder::getTotalAverageInputKilobitsPerSecond() const {
    float result = 0.0f;
    for (uint i=0; i<CHANNEL_COUNT; i++) {
        if (_channels[i]) {
            result += _channels[i]->getAverageInputKilobitsPerSecond();
        }
    }
    return result;
}

float BandwidthRecorder::getTotalAverageOutputKilobitsPerSecond() const {
    float result = 0.0f;
    for (uint i=0; i<CHANNEL_COUNT; i++) {
        if (_channels[i]) {
            result += _channels[i]->getAverageOutputKilobitsPerSecond();
        }
    }
    return result;
}

float BandwidthRecorder::getCachedTotalAverageInputPacketsPerSecond() const {
    static qint64 lastCalculated = 0;
    static float cachedValue = 0.0f;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastCalculated > 1000.0f) {
        lastCalculated = now;
        cachedValue = getTotalAverageInputPacketsPerSecond();
    }
    return cachedValue;
}

float BandwidthRecorder::getCachedTotalAverageOutputPacketsPerSecond() const {
    static qint64 lastCalculated = 0;
    static float cachedValue = 0.0f;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastCalculated > 1000.0f) {
        lastCalculated = now;
        cachedValue = getTotalAverageOutputPacketsPerSecond();
    }
    return cachedValue;
}

float BandwidthRecorder::getCachedTotalAverageInputKilobitsPerSecond() const {
    static qint64 lastCalculated = 0;
    static float cachedValue = 0.0f;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastCalculated > 1000.0f) {
        lastCalculated = now;
        cachedValue = getTotalAverageInputKilobitsPerSecond();
    }
    return cachedValue;
}

float BandwidthRecorder::getCachedTotalAverageOutputKilobitsPerSecond() const {
    static qint64 lastCalculated = 0;
    static float cachedValue = 0.0f;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastCalculated > 1000.0f) {
        lastCalculated = now;
        cachedValue = getTotalAverageOutputKilobitsPerSecond();
    }
    return cachedValue;
}
