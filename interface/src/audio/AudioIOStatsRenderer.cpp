//
//  AudioIOStatsRenderer.cpp
//  interface/src/audio
//
//  Created by Stephen Birarda on 2014-12-16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <AudioConstants.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <NodeList.h>
#include <Util.h>

#include "Audio.h"
#include "AudioIOStats.h"

#include "AudioIOStatsRenderer.h"

AudioIOStatsRenderer::AudioIOStatsRenderer() :
    _stats(NULL),
    _isEnabled(false),
    _shouldShowInjectedStreams(false)
{
    // grab the stats object from the audio I/O singleton
    _stats = &DependencyManager::get<Audio>()->getStats();
}

#ifdef _WIN32
const unsigned int STATS_WIDTH = 1500;
#else
const unsigned int STATS_WIDTH = 650;
#endif
const unsigned int STATS_HEIGHT_PER_LINE = 20;

void AudioIOStatsRenderer::render(const float* color, int width, int height) {
    if (!_isEnabled) {
        return;
    }
    
    const int linesWhenCentered = _shouldShowInjectedStreams ? 34 : 27;
    const int CENTERED_BACKGROUND_HEIGHT = STATS_HEIGHT_PER_LINE * linesWhenCentered;
    
    int lines = _shouldShowInjectedStreams ? _stats->getMixerInjectedStreamStatsMap().size() * 7 + 27 : 27;
    int statsHeight = STATS_HEIGHT_PER_LINE * lines;
    
    
    static const glm::vec4 backgroundColor = { 0.2f, 0.2f, 0.2f, 0.6f };
    int x = std::max((width - (int)STATS_WIDTH) / 2, 0);
    int y = std::max((height - CENTERED_BACKGROUND_HEIGHT) / 2, 0);
    int w = STATS_WIDTH;
    int h = statsHeight;
    DependencyManager::get<GeometryCache>()->renderQuad(x, y, w, h, backgroundColor);
    
    int horizontalOffset = x + 5;
    int verticalOffset = y;
    
    float scale = 0.10f;
    float rotation = 0.0f;
    int font = 2;
    
    char latencyStatString[512];
    
    float audioInputBufferLatency = 0.0f, inputRingBufferLatency = 0.0f, networkRoundtripLatency = 0.0f, mixerRingBufferLatency = 0.0f, outputRingBufferLatency = 0.0f, audioOutputBufferLatency = 0.0f;
    
    AudioStreamStats downstreamAudioStreamStats = _stats->getMixerDownstreamStats();
    SharedNodePointer audioMixerNodePointer = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::AudioMixer);
    if (!audioMixerNodePointer.isNull()) {
        audioInputBufferLatency = _stats->getAudioInputMsecsReadStats().getWindowAverage();
        inputRingBufferLatency = (float) _stats->getInputRungBufferMsecsAvailableStats().getWindowAverage();
        networkRoundtripLatency = audioMixerNodePointer->getPingMs();
        mixerRingBufferLatency = _stats->getMixerAvatarStreamStats()._framesAvailableAverage * AudioConstants::NETWORK_FRAME_MSECS;
        outputRingBufferLatency = downstreamAudioStreamStats._framesAvailableAverage * AudioConstants::NETWORK_FRAME_MSECS;
        audioOutputBufferLatency = _stats->getAudioOutputMsecsUnplayedStats().getWindowAverage();
    }
    float totalLatency = audioInputBufferLatency + inputRingBufferLatency + networkRoundtripLatency + mixerRingBufferLatency + outputRingBufferLatency + audioOutputBufferLatency;
    
    sprintf(latencyStatString, "     Audio input buffer: %7.2fms    - avg msecs of samples read to the input ring buffer in last 10s", audioInputBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "      Input ring buffer: %7.2fms    - avg msecs of samples in input ring buffer in last 10s", inputRingBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "       Network to mixer: %7.2fms    - half of last ping value calculated by the node list", networkRoundtripLatency / 2.0f);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, " AudioMixer ring buffer: %7.2fms    - avg msecs of samples in audio mixer's ring buffer in last 10s", mixerRingBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "      Network to client: %7.2fms    - half of last ping value calculated by the node list", networkRoundtripLatency / 2.0f);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "     Output ring buffer: %7.2fms    - avg msecs of samples in output ring buffer in last 10s", outputRingBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "    Audio output buffer: %7.2fms    - avg msecs of samples in audio output buffer in last 10s", audioOutputBufferLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    sprintf(latencyStatString, "                  TOTAL: %7.2fms\n", totalLatency);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, latencyStatString, color);
    
    
    verticalOffset += STATS_HEIGHT_PER_LINE;    // blank line
    
    char clientUpstreamMicLabelString[] = "Upstream Mic Audio Packets Sent Gaps (by client):";
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, clientUpstreamMicLabelString, color);
    
    const MovingMinMaxAvg<quint64>& packetSentTimeGaps = _stats->getPacketSentTimeGaps();
    
    char stringBuffer[512];
    sprintf(stringBuffer, "  Inter-packet timegaps (overall) | min: %9s, max: %9s, avg: %9s",
            formatUsecTime(packetSentTimeGaps.getMin()).toLatin1().data(),
            formatUsecTime(packetSentTimeGaps.getMax()).toLatin1().data(),
            formatUsecTime(packetSentTimeGaps.getAverage()).toLatin1().data());
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
    
    sprintf(stringBuffer, " Inter-packet timegaps (last 30s) | min: %9s, max: %9s, avg: %9s",
            formatUsecTime(packetSentTimeGaps.getWindowMin()).toLatin1().data(),
            formatUsecTime(packetSentTimeGaps.getWindowMax()).toLatin1().data(),
            formatUsecTime(packetSentTimeGaps.getWindowAverage()).toLatin1().data());
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
    
    verticalOffset += STATS_HEIGHT_PER_LINE;    // blank line
    
    char upstreamMicLabelString[] = "Upstream mic audio stats (received and reported by audio-mixer):";
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, upstreamMicLabelString, color);
    
    renderAudioStreamStats(&_stats->getMixerAvatarStreamStats(), horizontalOffset, verticalOffset,
                           scale, rotation, font, color);
    
    
    verticalOffset += STATS_HEIGHT_PER_LINE;    // blank line
    
    char downstreamLabelString[] = "Downstream mixed audio stats:";
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, downstreamLabelString, color);
    
    AudioStreamStats downstreamStats = _stats->getMixerDownstreamStats();
    renderAudioStreamStats(&downstreamStats, horizontalOffset, verticalOffset,
                           scale, rotation, font, color, true);
    
    
    if (_shouldShowInjectedStreams) {
        
        foreach(const AudioStreamStats& injectedStreamAudioStats, _stats->getMixerInjectedStreamStatsMap()) {
            
            verticalOffset += STATS_HEIGHT_PER_LINE;    // blank line
            
            char upstreamInjectedLabelString[512];
            sprintf(upstreamInjectedLabelString, "Upstream injected audio stats:      stream ID: %s",
                    injectedStreamAudioStats._streamIdentifier.toString().toLatin1().data());
            verticalOffset += STATS_HEIGHT_PER_LINE;
            drawText(horizontalOffset, verticalOffset, scale, rotation, font, upstreamInjectedLabelString, color);
            
            renderAudioStreamStats(&injectedStreamAudioStats, horizontalOffset, verticalOffset, scale, rotation, font, color);
        }
    }
}

void AudioIOStatsRenderer::renderAudioStreamStats(const AudioStreamStats* streamStats, int horizontalOffset, int& verticalOffset,
                                                float scale, float rotation, int font, const float* color, bool isDownstreamStats) {
    
    char stringBuffer[512];
    
    sprintf(stringBuffer, "                      Packet loss | overall: %5.2f%% (%d lost), last_30s: %5.2f%% (%d lost)",
            streamStats->_packetStreamStats.getLostRate() * 100.0f,
            streamStats->_packetStreamStats._lost,
            streamStats->_packetStreamWindowStats.getLostRate() * 100.0f,
            streamStats->_packetStreamWindowStats._lost);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
    
    if (isDownstreamStats) {
        sprintf(stringBuffer, "                Ringbuffer frames | desired: %u, avg_available(10s): %u+%d, available: %u+%d",
                streamStats->_desiredJitterBufferFrames,
                streamStats->_framesAvailableAverage,
                (int)(_stats->getAudioInputMsecsReadStats().getWindowAverage() / AudioConstants::NETWORK_FRAME_MSECS),
                streamStats->_framesAvailable,
                (int)(_stats->getAudioOutputMsecsUnplayedStats().getCurrentIntervalLastSample()
                      / AudioConstants::NETWORK_FRAME_MSECS));
    } else {
        sprintf(stringBuffer, "                Ringbuffer frames | desired: %u, avg_available(10s): %u, available: %u",
                streamStats->_desiredJitterBufferFrames,
                streamStats->_framesAvailableAverage,
                streamStats->_framesAvailable);
    }
    
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
    
    sprintf(stringBuffer, "                 Ringbuffer stats | starves: %u, prev_starve_lasted: %u, frames_dropped: %u, overflows: %u",
            streamStats->_starveCount,
            streamStats->_consecutiveNotMixedCount,
            streamStats->_framesDropped,
            streamStats->_overflowCount);
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
    
    sprintf(stringBuffer, "  Inter-packet timegaps (overall) | min: %9s, max: %9s, avg: %9s",
            formatUsecTime(streamStats->_timeGapMin).toLatin1().data(),
            formatUsecTime(streamStats->_timeGapMax).toLatin1().data(),
            formatUsecTime(streamStats->_timeGapAverage).toLatin1().data());
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
    
    sprintf(stringBuffer, " Inter-packet timegaps (last 30s) | min: %9s, max: %9s, avg: %9s",
            formatUsecTime(streamStats->_timeGapWindowMin).toLatin1().data(),
            formatUsecTime(streamStats->_timeGapWindowMax).toLatin1().data(),
            formatUsecTime(streamStats->_timeGapWindowAverage).toLatin1().data());
    verticalOffset += STATS_HEIGHT_PER_LINE;
    drawText(horizontalOffset, verticalOffset, scale, rotation, font, stringBuffer, color);
}