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

// This is called 1x/sec (see AudioClient) and we want it to log the last 5s
static const int INPUT_READS_WINDOW = 5;
static const int INPUT_UNPLAYED_WINDOW = 5;
static const int OUTPUT_UNPLAYED_WINDOW = 5;

static const int APPROXIMATELY_30_SECONDS_OF_AUDIO_PACKETS = (int)(30.0f * 1000.0f / AudioConstants::NETWORK_FRAME_MSECS);


AudioIOStats::AudioIOStats(MixedProcessedAudioStream* receivedAudioStream) :
    _interface(new AudioStatsInterface(this)),
    _inputMsRead(1, INPUT_READS_WINDOW),
    _inputMsUnplayed(1, INPUT_UNPLAYED_WINDOW),
    _outputMsUnplayed(1, OUTPUT_UNPLAYED_WINDOW),
    _lastSentPacketTime(0),
    _packetTimegaps(1, APPROXIMATELY_30_SECONDS_OF_AUDIO_PACKETS),
    _receivedAudioStream(receivedAudioStream)
{

}

void AudioIOStats::reset() {
    _receivedAudioStream->resetStats();

    _inputMsRead.reset();
    _inputMsUnplayed.reset();
    _outputMsUnplayed.reset();
    _packetTimegaps.reset();

    _interface->updateLocalBuffers(_inputMsRead, _inputMsUnplayed, _outputMsUnplayed, _packetTimegaps);
    _interface->updateMixerStream(AudioStreamStats());
    _interface->updateClientStream(AudioStreamStats());
    _interface->updateInjectorStreams(QHash<QUuid, AudioStreamStats>());
}

void AudioIOStats::sentPacket() const {
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

void AudioIOStats::processStreamStatsPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer sendingNode) {
    // parse the appendFlag, clear injected audio stream stats if 0
    quint8 appendFlag;
    message->readPrimitive(&appendFlag);

    if (appendFlag == 0) {
        _injectorStreams.clear();
    }

    // parse the number of stream stats structs to follow
    quint16 numStreamStats;
    message->readPrimitive(&numStreamStats);

    // parse the stream stats
    AudioStreamStats streamStats;
    for (quint16 i = 0; i < numStreamStats; i++) {
        message->readPrimitive(&streamStats);

        if (streamStats._streamType == PositionalAudioStream::Microphone) {
            _interface->updateMixerStream(streamStats);
        } else {
            _injectorStreams[streamStats._streamIdentifier] = streamStats;
        }
    }

    if (appendFlag == 2) {
        _interface->updateInjectorStreams(_injectorStreams);
    }
}

void AudioIOStats::publish() {
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

    // update the interface
    _interface->updateLocalBuffers(_inputMsRead, _inputMsUnplayed, _outputMsUnplayed, _packetTimegaps);
    _interface->updateClientStream(stats);

    // prepare a packet to the mixer
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

AudioStreamStatsInterface::AudioStreamStatsInterface(QObject* parent) :
    QObject(parent) {}

void AudioStreamStatsInterface::updateStream(const AudioStreamStats& stats) {
    lossRate(stats._packetStreamStats.getLostRate());
    lossCount(stats._packetStreamStats._lost);
    lossRateWindow(stats._packetStreamWindowStats.getLostRate());
    lossCountWindow(stats._packetStreamWindowStats._lost);

    framesDesired(stats._desiredJitterBufferFrames);
    framesAvailable(stats._framesAvailable);
    framesAvailableAvg(stats._framesAvailableAverage);

    unplayedMsMax(stats._unplayedMs);

    starveCount(stats._starveCount);
    lastStarveDurationCount(stats._consecutiveNotMixedCount);
    dropCount(stats._framesDropped);
    overflowCount(stats._overflowCount);

    timegapMsMax(stats._timeGapMax / USECS_PER_MSEC);
    timegapMsAvg(stats._timeGapAverage / USECS_PER_MSEC);
    timegapMsMaxWindow(stats._timeGapWindowMax / USECS_PER_MSEC);
    timegapMsAvgWindow(stats._timeGapWindowAverage / USECS_PER_MSEC);
}

AudioStatsInterface::AudioStatsInterface(QObject* parent) :
    QObject(parent),
    _client(new AudioStreamStatsInterface(this)),
    _mixer(new AudioStreamStatsInterface(this)),
    _injectors(new QObject(this)) {}


void AudioStatsInterface::updateLocalBuffers(const MovingMinMaxAvg<float>& inputMsRead,
    const MovingMinMaxAvg<float>& inputMsUnplayed,
    const MovingMinMaxAvg<float>& outputMsUnplayed,
    const MovingMinMaxAvg<quint64>& timegaps) {
    if (SharedNodePointer audioNode = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::AudioMixer)) {
        pingMs(audioNode->getPingMs());
    }

    inputReadMsMax(inputMsRead.getWindowMax());
    inputUnplayedMsMax(inputMsUnplayed.getWindowMax());
    outputUnplayedMsMax(outputMsUnplayed.getWindowMax());

    sentTimegapMsMax(timegaps.getMax() / USECS_PER_MSEC);
    sentTimegapMsAvg(timegaps.getAverage() / USECS_PER_MSEC);
    sentTimegapMsMaxWindow(timegaps.getWindowMax() / USECS_PER_MSEC);
    sentTimegapMsAvgWindow(timegaps.getWindowAverage() / USECS_PER_MSEC);
}

void AudioStatsInterface::updateInjectorStreams(const QHash<QUuid, AudioStreamStats>& stats) {
    // TODO
    emit injectorStreamsChanged();
}
