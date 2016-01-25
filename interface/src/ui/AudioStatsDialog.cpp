//
//  AudioStatsDialog.cpp
//  interface/src/ui
//
//  Created by Bridget Went on 7/9/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioStatsDialog.h"

#include <cstdio>

#include <AudioClient.h>
#include <AudioConstants.h>
#include <AudioIOStats.h>
#include <DependencyManager.h>
#include <GeometryCache.h>
#include <NodeList.h>
#include <Util.h>



const unsigned COLOR0 = 0x33cc99ff;
const unsigned COLOR1 = 0xffef40c0;
const unsigned COLOR2 = 0xd0d0d0a0;
const unsigned COLOR3 = 0x01DD7880;


AudioStatsDisplay::AudioStatsDisplay(QFormLayout* form,
                                     QString text, unsigned colorRGBA) :
_text(text),
_colorRGBA(colorRGBA)
{
    _label = new QLabel();
    _label->setAlignment(Qt::AlignCenter);
    
    QPalette palette = _label->palette();
    unsigned rgb = colorRGBA >> 8;
    rgb = ((rgb & 0xfefefeu) >> 1) + ((rgb & 0xf8f8f8) >> 3);
    palette.setColor(QPalette::WindowText, QColor::fromRgb(rgb));
    _label->setPalette(palette);
    
    form->addRow(_label);
}

void AudioStatsDisplay::paint() {
    _label->setText(_strBuf);
}

void AudioStatsDisplay::updatedDisplay(QString str) {
    _strBuf = str;
}


AudioStatsDialog::AudioStatsDialog(QWidget* parent) :
    QDialog(parent, Qt::Window | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint) {
    
    _shouldShowInjectedStreams = false;
    
    setWindowTitle("Audio Network Statistics");
    
    // Get statistics from the Audio Client
    _stats = &DependencyManager::get<AudioClient>()->getStats();
        
    // Create layout
    _form = new QFormLayout();
    _form->setSizeConstraint(QLayout::SetFixedSize);
    QDialog::setLayout(_form);
        
    // Load and initialize all channels
    renderStats();

    _audioDisplayChannels = QVector<QVector<AudioStatsDisplay*>>(1);
        
    _audioMixerID = addChannel(_form, _audioMixerStats, COLOR0);
    _upstreamClientID = addChannel(_form, _upstreamClientStats, COLOR1);
    _upstreamMixerID = addChannel(_form, _upstreamMixerStats, COLOR2);
    _downstreamID = addChannel(_form, _downstreamStats, COLOR3);
    _upstreamInjectedID = addChannel(_form, _upstreamInjectedStats, COLOR0);
        
    connect(averageUpdateTimer, SIGNAL(timeout()), this, SLOT(updateTimerTimeout()));
    averageUpdateTimer->start(1000);
}

int AudioStatsDialog::addChannel(QFormLayout* form, QVector<QString>& stats, const unsigned color) {
    
    int channelID = _audioDisplayChannels.size() - 1;
    
    for (int i = 0; i < stats.size(); i++)
        // Create new display label
        _audioDisplayChannels[channelID].push_back(new AudioStatsDisplay(form, stats.at(i), color));
    
    // Expand vector to fit next channel
    _audioDisplayChannels.resize(_audioDisplayChannels.size() + 1);
    
    return channelID;
}

void AudioStatsDialog::updateStats(QVector<QString>& stats, int channelID) {
    // Update all stat displays at specified channel
    for (int i = 0; i < stats.size(); i++)
        _audioDisplayChannels[channelID].at(i)->updatedDisplay(stats.at(i));
}

void AudioStatsDialog::renderStats() {

    // Clear current stats from all vectors
    clearAllChannels();
    
    double audioInputBufferLatency = 0.0,
           inputRingBufferLatency = 0.0,
           networkRoundtripLatency = 0.0,
           mixerRingBufferLatency = 0.0,
           outputRingBufferLatency = 0.0,
           audioOutputBufferLatency = 0.0;
        
    AudioStreamStats downstreamAudioStreamStats = _stats->getMixerDownstreamStats();
    SharedNodePointer audioMixerNodePointer = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::AudioMixer);
    
    if (!audioMixerNodePointer.isNull()) {
        audioInputBufferLatency = (double)_stats->getAudioInputMsecsReadStats().getWindowAverage();
        inputRingBufferLatency =  (double)_stats->getInputRungBufferMsecsAvailableStats().getWindowAverage();
        networkRoundtripLatency = (double) audioMixerNodePointer->getPingMs();
        mixerRingBufferLatency = (double)_stats->getMixerAvatarStreamStats()._framesAvailableAverage *
            (double)AudioConstants::NETWORK_FRAME_MSECS;
        outputRingBufferLatency = (double)downstreamAudioStreamStats._framesAvailableAverage *
            (double)AudioConstants::NETWORK_FRAME_MSECS;
        audioOutputBufferLatency = (double)_stats->getAudioOutputMsecsUnplayedStats().getWindowAverage();
    }
        
    double totalLatency = audioInputBufferLatency + inputRingBufferLatency + networkRoundtripLatency + mixerRingBufferLatency
        + outputRingBufferLatency + audioOutputBufferLatency;

    QString stats = "Audio input buffer: %1ms   - avg msecs of samples read to the audio input buffer in last 10s";
    _audioMixerStats.push_back(stats.arg(QString::number(audioInputBufferLatency, 'f', 2)));

    stats = "Input ring buffer: %1ms  - avg msecs of samples read to the input ring buffer in last 10s";
    _audioMixerStats.push_back(stats.arg(QString::number(inputRingBufferLatency, 'f', 2)));
    stats = "Network to mixer: %1ms  - half of last ping value calculated by the node list";
    _audioMixerStats.push_back(stats.arg(QString::number((networkRoundtripLatency / 2.0), 'f', 2)));
    stats = "Network to client: %1ms  - half of last ping value calculated by the node list";
    _audioMixerStats.push_back(stats.arg(QString::number((mixerRingBufferLatency / 2.0),'f', 2)));
    stats = "Output ring buffer: %1ms  - avg msecs of samples in output ring buffer in last 10s";
    _audioMixerStats.push_back(stats.arg(QString::number(outputRingBufferLatency,'f', 2)));
    stats = "Audio output buffer: %1ms  - avg msecs of samples in audio output buffer in last 10s";
    _audioMixerStats.push_back(stats.arg(QString::number(mixerRingBufferLatency,'f', 2)));
    stats = "TOTAL: %1ms  - avg msecs of samples in audio output buffer in last 10s";
    _audioMixerStats.push_back(stats.arg(QString::number(totalLatency, 'f', 2)));


    const MovingMinMaxAvg<quint64>& packetSentTimeGaps = _stats->getPacketSentTimeGaps();

    _upstreamClientStats.push_back("\nUpstream Mic Audio Packets Sent Gaps (by client):");

    stats = "Inter-packet timegaps (overall) | min: %1, max: %2, avg: %3";
    stats = stats.arg(formatUsecTime(packetSentTimeGaps.getMin()),
                      formatUsecTime(packetSentTimeGaps.getMax()),
                      formatUsecTime(packetSentTimeGaps.getAverage()));
    _upstreamClientStats.push_back(stats);

    stats = "Inter-packet timegaps (last 30s) | min: %1, max: %2, avg: %3";
    stats = stats.arg(formatUsecTime(packetSentTimeGaps.getWindowMin()),
                      formatUsecTime(packetSentTimeGaps.getWindowMax()),
                      formatUsecTime(packetSentTimeGaps.getWindowAverage()));
    _upstreamClientStats.push_back(stats);
    
    _upstreamMixerStats.push_back("\nUpstream mic audio stats (received and reported by audio-mixer):");
        
    renderAudioStreamStats(&_stats->getMixerAvatarStreamStats(), &_upstreamMixerStats, true);
    
    _downstreamStats.push_back("\nDownstream mixed audio stats:");
        
    AudioStreamStats downstreamStats = _stats->getMixerDownstreamStats();
    
    renderAudioStreamStats(&downstreamStats, &_downstreamStats, true);
   
    
    if (_shouldShowInjectedStreams) {

        foreach(const AudioStreamStats& injectedStreamAudioStats, _stats->getMixerInjectedStreamStatsMap()) {
            stats = "\nUpstream injected audio stats:      stream ID: %1";
            stats = stats.arg(injectedStreamAudioStats._streamIdentifier.toString());
            _upstreamInjectedStats.push_back(stats);
            
            renderAudioStreamStats(&injectedStreamAudioStats, &_upstreamInjectedStats, true);
        }
 
    }
}


void AudioStatsDialog::renderAudioStreamStats(const AudioStreamStats* streamStats, QVector<QString>* audioStreamStats, bool isDownstreamStats) {

    QString stats = "Packet loss | overall: %1% (%2 lost),      last_30s: %3% (%4 lost)";
    stats = stats.arg(QString::number((int)(streamStats->_packetStreamStats.getLostRate() * 100.0f)),
                      QString::number((int)(streamStats->_packetStreamStats._lost)),
                      QString::number((int)(streamStats->_packetStreamWindowStats.getLostRate() * 100.0f)),
                      QString::number((int)(streamStats->_packetStreamWindowStats._lost)));
    audioStreamStats->push_back(stats);

    if (isDownstreamStats) {
        stats = "Ringbuffer frames | desired: %1, avg_available(10s): %2 + %3, available: %4 + %5";
        stats = stats.arg(QString::number(streamStats->_desiredJitterBufferFrames),
                          QString::number(streamStats->_framesAvailableAverage),
                          QString::number((int)((float)_stats->getAudioInputMsecsReadStats().getWindowAverage() /
                                                AudioConstants::NETWORK_FRAME_MSECS)),
                          QString::number(streamStats->_framesAvailable),
                          QString::number((int)(_stats->getAudioOutputMsecsUnplayedStats().getCurrentIntervalLastSample() /
                                                AudioConstants::NETWORK_FRAME_MSECS)));
        audioStreamStats->push_back(stats);
    } else {
        stats = "Ringbuffer frames | desired: %1, avg_available(10s): %2, available: %3";
        stats = stats.arg(QString::number(streamStats->_desiredJitterBufferFrames),
                          QString::number(streamStats->_framesAvailableAverage),
                          QString::number(streamStats->_framesAvailable));
        audioStreamStats->push_back(stats);
    }


    stats = "Ringbuffer stats | starves: %1, prev_starve_lasted: %2, frames_dropped: %3, overflows: %4";
    stats = stats.arg(QString::number(streamStats->_starveCount),
                      QString::number(streamStats->_consecutiveNotMixedCount),
                      QString::number(streamStats->_framesDropped),
                      QString::number(streamStats->_overflowCount));
    audioStreamStats->push_back(stats);


    stats = "Inter-packet timegaps (overall) | min: %1, max: %2, avg: %3";
    stats = stats.arg(formatUsecTime(streamStats->_timeGapMin),
                      formatUsecTime(streamStats->_timeGapMax),
                      formatUsecTime(streamStats->_timeGapAverage));
    audioStreamStats->push_back(stats);


    stats = "Inter-packet timegaps (last 30s) | min: %1, max: %2, avg: %3";
    stats = stats.arg(formatUsecTime(streamStats->_timeGapWindowMin),
                      formatUsecTime(streamStats->_timeGapWindowMax),
                      formatUsecTime(streamStats->_timeGapWindowAverage));
    audioStreamStats->push_back(stats);
    
}

void AudioStatsDialog::clearAllChannels() {
    _audioMixerStats.clear();
    _upstreamClientStats.clear();
    _upstreamMixerStats.clear();
    _downstreamStats.clear();
    _upstreamInjectedStats.clear();
}


void AudioStatsDialog::updateTimerTimeout() {
    
    renderStats();
    
    // Update all audio stats
    updateStats(_audioMixerStats, _audioMixerID);
    updateStats(_upstreamClientStats, _upstreamClientID);
    updateStats(_upstreamMixerStats, _upstreamMixerID);
    updateStats(_downstreamStats, _downstreamID);
    updateStats(_upstreamInjectedStats, _upstreamInjectedID);
    
}


void AudioStatsDialog::paintEvent(QPaintEvent* event) {
    
    // Repaint each stat in each channel
    for (int i = 0; i < _audioDisplayChannels.size(); i++) {
        for(int j = 0; j < _audioDisplayChannels[i].size(); j++) {
            _audioDisplayChannels[i].at(j)->paint();
        }
    }
    
    QDialog::paintEvent(event);
}

void AudioStatsDialog::reject() {
    // Just regularly close upon ESC
    QDialog::close();
}

void AudioStatsDialog::closeEvent(QCloseEvent* event) {
    QDialog::closeEvent(event);
    emit closed();
}

AudioStatsDialog::~AudioStatsDialog() {
    clearAllChannels();
    for (int i = 0; i < _audioDisplayChannels.size(); i++) {
        _audioDisplayChannels[i].clear();
        for(int j = 0; j < _audioDisplayChannels[i].size(); j++) {
            delete _audioDisplayChannels[i].at(j);
        }
    }
    
}


