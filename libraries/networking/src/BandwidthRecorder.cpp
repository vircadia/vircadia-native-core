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


BandwidthRecorder::Channel::Channel() {
}

float BandwidthRecorder::Channel::getAverageInputPacketsPerSecond() {
    float delt = _input.getEventDeltaAverage();
    if (delt > 0.)
        return (1.0 / delt);
    return 0.;
}

float BandwidthRecorder::Channel::getAverageOutputPacketsPerSecond() {
    float delt = _input.getEventDeltaAverage();
    if (delt > 0.)
        return (1.0 / _output.getEventDeltaAverage());
    return 0.;
}

float BandwidthRecorder::Channel::getAverageInputKilobitsPerSecond() {
    return (_input.getAverageSampleValuePerSecond() * (8.0f / 1000));
}

float BandwidthRecorder::Channel::getAverageOutputKilobitsPerSecond() {
    return (_output.getAverageSampleValuePerSecond() * (8.0f / 1000));
}


void BandwidthRecorder::Channel::updateInputAverage(const float sample) {
    _input.updateAverage(sample);
}

void BandwidthRecorder::Channel::updateOutputAverage(const float sample) {
    _output.updateAverage(sample);
}



BandwidthRecorder::BandwidthRecorder() {
}


BandwidthRecorder::~BandwidthRecorder() {
}



void BandwidthRecorder::updateInboundData(const quint8 channelType, const int sample) {

    totalChannel.updateInputAverage(sample);

    // see Node.h NodeType
    switch (channelType) {
    case NodeType::DomainServer:
    case NodeType::EntityServer:
        octreeChannel.updateInputAverage(sample);
        break;
    case NodeType::MetavoxelServer:
    case NodeType::EnvironmentServer:
        metavoxelsChannel.updateInputAverage(sample);
        break;
    case NodeType::AudioMixer:
        audioChannel.updateInputAverage(sample);
        break;
    case NodeType::Agent:
    case NodeType::AvatarMixer:
        avatarsChannel.updateInputAverage(sample);
        break;
    case NodeType::Unassigned:
    default:
        otherChannel.updateInputAverage(sample);
        break;
    }
}

void BandwidthRecorder::updateOutboundData(const quint8 channelType, const int sample) {

    totalChannel.updateOutputAverage(sample);

    // see Node.h NodeType
    switch (channelType) {
    case NodeType::DomainServer:
    case NodeType::EntityServer:
        octreeChannel.updateOutputAverage(sample);
        break;
    case NodeType::MetavoxelServer:
    case NodeType::EnvironmentServer:
        metavoxelsChannel.updateOutputAverage(sample);
        break;
    case NodeType::AudioMixer:
        audioChannel.updateOutputAverage(sample);
        break;
    case NodeType::Agent:
    case NodeType::AvatarMixer:
        avatarsChannel.updateOutputAverage(sample);
        break;
    case NodeType::Unassigned:
    default:
        otherChannel.updateOutputAverage(sample);
        break;
    }
}
