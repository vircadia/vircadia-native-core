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

const int FRAMES_AVAILABLE_STATS_WINDOW_SECONDS = 10;

const int APPROXIMATELY_30_SECONDS_OF_AUDIO_PACKETS = (int)(30.0f * 1000.0f / AudioConstants::NETWORK_FRAME_MSECS);


AudioIOStats::AudioIOStats(MixedProcessedAudioStream* receivedAudioStream) :
    _receivedAudioStream(receivedAudioStream),
    _audioInputMsecsReadStats(MSECS_PER_SECOND / (float)AudioConstants::NETWORK_FRAME_MSECS * AudioClient::CALLBACK_ACCELERATOR_RATIO, FRAMES_AVAILABLE_STATS_WINDOW_SECONDS),
    _inputRingBufferMsecsAvailableStats(1, FRAMES_AVAILABLE_STATS_WINDOW_SECONDS),
    _audioOutputMsecsUnplayedStats(1, FRAMES_AVAILABLE_STATS_WINDOW_SECONDS),
    _lastSentAudioPacket(0),
    _packetSentTimeGaps(1, APPROXIMATELY_30_SECONDS_OF_AUDIO_PACKETS)
{

}

AudioStreamStats AudioIOStats::getMixerDownstreamStats() const {
    return _receivedAudioStream->getAudioStreamStats();
}

void AudioIOStats::reset() {
    _receivedAudioStream->resetStats();

    _mixerAvatarStreamStats = AudioStreamStats();
    _mixerInjectedStreamStatsMap.clear();

    _audioInputMsecsReadStats.reset();
    _inputRingBufferMsecsAvailableStats.reset();

    _audioOutputMsecsUnplayedStats.reset();
    _packetSentTimeGaps.reset();
}

void AudioIOStats::sentPacket() {
    // first time this is 0
    if (_lastSentAudioPacket == 0) {
        _lastSentAudioPacket = usecTimestampNow();
    } else {
        quint64 now = usecTimestampNow();
        quint64 gap = now - _lastSentAudioPacket;
        _packetSentTimeGaps.update(gap);

        _lastSentAudioPacket = now;
    }
}
void AudioIOStats::processStreamStatsPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode) {

    // parse the appendFlag, clear injected audio stream stats if 0
    quint8 appendFlag;
    packet->readPrimitive(&appendFlag);

    if (!appendFlag) {
        _mixerInjectedStreamStatsMap.clear();
    }

    // parse the number of stream stats structs to follow
    quint16 numStreamStats;
    packet->readPrimitive(&numStreamStats);

    // parse the stream stats
    AudioStreamStats streamStats;
    for (quint16 i = 0; i < numStreamStats; i++) {
        packet->readPrimitive(&streamStats);

        if (streamStats._streamType == PositionalAudioStream::Microphone) {
            _mixerAvatarStreamStats = streamStats;
        } else {
            _mixerInjectedStreamStatsMap[streamStats._streamIdentifier] = streamStats;
        }
    }
}

void AudioIOStats::sendDownstreamAudioStatsPacket() {

    auto audioIO = DependencyManager::get<AudioClient>();

    // since this function is called every second, we'll sample for some of our stats here
    _inputRingBufferMsecsAvailableStats.update(audioIO->getInputRingBufferMsecsAvailable());
    _audioOutputMsecsUnplayedStats.update(audioIO->getAudioOutputMsecsUnplayed());

    // also, call _receivedAudioStream's per-second callback
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
