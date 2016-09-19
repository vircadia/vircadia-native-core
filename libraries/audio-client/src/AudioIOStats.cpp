//
//  AudioStats.cpp
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <AudioConstants.h>
#include <MixedProcessedAudioStream.h>
#include <NodeList.h>
#include <PositionalAudioStream.h>

#include "AudioClient.h"

#include "AudioIOStats.h"

// This is called 5x/sec (see AudioStatsDialog), and we want it to log the last 5s
static const int INPUT_READS_WINDOW = 25;
static const int INPUT_UNPLAYED_WINDOW = 25;
static const int OUTPUT_UNPLAYED_WINDOW = 25;

static const int APPROXIMATELY_30_SECONDS_OF_AUDIO_PACKETS = (int)(30.0f * 1000.0f / AudioConstants::NETWORK_FRAME_MSECS);


AudioIOStats::AudioIOStats(MixedProcessedAudioStream* receivedAudioStream) :
    _receivedAudioStream(receivedAudioStream),
    _inputMsRead(0, INPUT_READS_WINDOW),
    _inputMsUnplayed(0, INPUT_UNPLAYED_WINDOW),
    _outputMsUnplayed(0, OUTPUT_UNPLAYED_WINDOW),
    _lastSentPacketTime(0),
    _packetTimegaps(0, APPROXIMATELY_30_SECONDS_OF_AUDIO_PACKETS)
{
}

void AudioIOStats::reset() {
    _receivedAudioStream->resetStats();

    _inputMsRead.reset();
    _inputMsUnplayed.reset();
    _outputMsUnplayed.reset();
    _packetTimegaps.reset();

    _mixerAvatarStreamStats = AudioStreamStats();
    _mixerInjectedStreamStatsMap.clear();
}

void AudioIOStats::sentPacket() {
    // first time this is 0
    if (_lastSentPacketTime == 0) {
        _lastSentPacketTime = usecTimestampNow();
    } else {
        quint64 now = usecTimestampNow();
        quint64 gap = now - _lastSentPacketTime;
        _lastSentPacketTime = now;
        _packetTimegaps.update(gap);
    }
}

const MovingMinMaxAvg<float>& AudioIOStats::getInputMsRead() const {
    _inputMsRead.currentIntervalComplete();
    return _inputMsRead;
}

const MovingMinMaxAvg<float>& AudioIOStats::getInputMsUnplayed() const {
    _inputMsUnplayed.currentIntervalComplete();
    return _inputMsUnplayed;
}

const MovingMinMaxAvg<float>& AudioIOStats::getOutputMsUnplayed() const {
    _outputMsUnplayed.currentIntervalComplete();
    return _outputMsUnplayed;
}

const MovingMinMaxAvg<quint64>& AudioIOStats::getPacketTimegaps() const {
    _packetTimegaps.currentIntervalComplete();
    return _packetTimegaps;
}

const AudioStreamStats AudioIOStats::getMixerDownstreamStats() const {
    return _receivedAudioStream->getAudioStreamStats();
}

void AudioIOStats::processStreamStatsPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // parse the appendFlag, clear injected audio stream stats if 0
    quint8 appendFlag;
    message->readPrimitive(&appendFlag);

    if (!appendFlag) {
        _mixerInjectedStreamStatsMap.clear();
    }

    // parse the number of stream stats structs to follow
    quint16 numStreamStats;
    message->readPrimitive(&numStreamStats);

    // parse the stream stats
    AudioStreamStats streamStats;
    for (quint16 i = 0; i < numStreamStats; i++) {
        message->readPrimitive(&streamStats);

        if (streamStats._streamType == PositionalAudioStream::Microphone) {
            _mixerAvatarStreamStats = streamStats;
        } else {
            _mixerInjectedStreamStatsMap[streamStats._streamIdentifier] = streamStats;
        }
    }
}

void AudioIOStats::sendDownstreamAudioStatsPacket() {
    auto audioIO = DependencyManager::get<AudioClient>();

    // call _receivedAudioStream's per-second callback
    _receivedAudioStream->perSecondCallbackForUpdatingStats();

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
    if (!audioMixer) {
        return;
    }

    quint8 appendFlag = 0;
    quint16 numStreamStatsToPack = 1;
    AudioStreamStats stats = _receivedAudioStream->getAudioStreamStats();

    int statsPacketSize = sizeof(appendFlag) + sizeof(numStreamStatsToPack) + sizeof(stats);
    auto statsPacket = NLPacket::create(PacketType::AudioStreamStats, statsPacketSize);

    // pack append flag
    statsPacket->writePrimitive(appendFlag);

    // pack number of stats packed
    statsPacket->writePrimitive(numStreamStatsToPack);

    // pack downstream audio stream stats
    statsPacket->writePrimitive(stats);
    
    // send packet
    nodeList->sendPacket(std::move(statsPacket), *audioMixer);
}
