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
        
    // Create layouter
    _form = new QFormLayout();
    QDialog::setLayout(_form);
        
    // Load and initilize all channels
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
        mixerRingBufferLatency = (double)_stats->getMixerAvatarStreamStats()._framesAvailableAverage * AudioConstants::NETWORK_FRAME_MSECS;
        outputRingBufferLatency = (double)downstreamAudioStreamStats._framesAvailableAverage * AudioConstants::NETWORK_FRAME_MSECS;
        audioOutputBufferLatency = (double)_stats->getAudioOutputMsecsUnplayedStats().getWindowAverage();
    }
        
    double totalLatency = audioInputBufferLatency + inputRingBufferLatency + networkRoundtripLatency + mixerRingBufferLatency
        + outputRingBufferLatency + audioOutputBufferLatency;
        
    _audioMixerStats.push_back(QString("Audio input buffer: %1ms").arg(
                                                                       QString::number(audioInputBufferLatency, 'f', 2)) + QString("   - avg msecs of samples read to the audio input buffer in last 10s"));
    
    _audioMixerStats.push_back(QString("Input ring buffer: %1ms").arg(
                                                                      QString::number(inputRingBufferLatency, 'f', 2)) + QString("  - avg msecs of samples read to the input ring buffer in last 10s"));
    _audioMixerStats.push_back(QString("Network to mixer: %1ms").arg(
                                                                     QString::number((networkRoundtripLatency / 2.0), 'f', 2)) + QString("  - half of last ping value calculated by the node list"));
    _audioMixerStats.push_back(QString("Network to client: %1ms").arg(
                                                                      QString::number((mixerRingBufferLatency / 2.0),'f', 2)) + QString("  - half of last ping value calculated by the node list"));
    _audioMixerStats.push_back(QString("Output ring buffer: %1ms").arg(
                                                                       QString::number(outputRingBufferLatency,'f', 2)) + QString("  - avg msecs of samples in output ring buffer in last 10s"));
    _audioMixerStats.push_back(QString("Audio output buffer: %1ms").arg(
                                                                        QString::number(mixerRingBufferLatency,'f', 2)) + QString("  - avg msecs of samples in audio output buffer in last 10s"));
    _audioMixerStats.push_back(QString("TOTAL: %1ms").arg(
                                                          QString::number(totalLatency, 'f', 2)) +QString("  - avg msecs of samples in audio output buffer in last 10s"));
    
       
    const MovingMinMaxAvg<quint64>& packetSentTimeGaps = _stats->getPacketSentTimeGaps();
    
     _upstreamClientStats.push_back(
                                     QString("\nUpstream Mic Audio Packets Sent Gaps (by client):"));
        
    _upstreamClientStats.push_back(
                                    QString("Inter-packet timegaps (overall) | min: %1, max: %2, avg: %3").arg(formatUsecTime(packetSentTimeGaps.getMin()).toLatin1().data()).arg(formatUsecTime(                                                                            packetSentTimeGaps.getMax()).toLatin1().data()).arg(formatUsecTime(                                                                                                                                                                                                                                                     packetSentTimeGaps.getAverage()).toLatin1().data()));
    _upstreamClientStats.push_back(
                                    QString("Inter-packet timegaps (last 30s) | min: %1, max: %2, avg: %3").arg(formatUsecTime(packetSentTimeGaps.getWindowMin()).toLatin1().data()).arg(formatUsecTime(packetSentTimeGaps.getWindowMax()).toLatin1().data()).arg(formatUsecTime(packetSentTimeGaps.getWindowAverage()).toLatin1().data()));
    
    _upstreamMixerStats.push_back(QString("\nUpstream mic audio stats (received and reported by audio-mixer):"));
        
    renderAudioStreamStats(&_stats->getMixerAvatarStreamStats(), &_upstreamMixerStats, true);
    
    _downstreamStats.push_back(QString("\nDownstream mixed audio stats:"));
        
    AudioStreamStats downstreamStats = _stats->getMixerDownstreamStats();
    
    renderAudioStreamStats(&downstreamStats, &_downstreamStats, true);
   
    
    if (_shouldShowInjectedStreams) {

        foreach(const AudioStreamStats& injectedStreamAudioStats, _stats->getMixerInjectedStreamStatsMap()) {
            
            _upstreamInjectedStats.push_back(QString("\nUpstream injected audio stats:      stream ID: %1").arg(                    injectedStreamAudioStats._streamIdentifier.toString().toLatin1().data()));
            
            renderAudioStreamStats(&injectedStreamAudioStats, &_upstreamInjectedStats, true);
        }
 
    }
}


void AudioStatsDialog::renderAudioStreamStats(const AudioStreamStats* streamStats, QVector<QString>* audioStreamStats, bool isDownstreamStats) {
    
    
    audioStreamStats->push_back(
                                QString("Packet loss | overall: %1% (%2 lost),      last_30s: %3% (%4 lost)").arg(QString::number((int)(streamStats->_packetStreamStats.getLostRate() * 100.0f))).arg(QString::number((int)(streamStats->_packetStreamStats._lost))).arg(QString::number((int)(streamStats->_packetStreamWindowStats.getLostRate() * 100.0f))).arg(QString::number((int)(streamStats->_packetStreamWindowStats._lost)))
                                );
   
    if (isDownstreamStats) {
        audioStreamStats->push_back(
                                    QString("Ringbuffer frames | desired: %1, avg_available(10s): %2 + %3, available: %4+%5").arg(QString::number(streamStats->_desiredJitterBufferFrames)).arg(QString::number(streamStats->_framesAvailableAverage)).arg(QString::number((int)((float)_stats->getAudioInputMsecsReadStats().getWindowAverage() / AudioConstants::NETWORK_FRAME_MSECS))).arg(QString::number(streamStats->_framesAvailable)).arg(QString::number((int)(_stats->getAudioOutputMsecsUnplayedStats().getCurrentIntervalLastSample() / AudioConstants::NETWORK_FRAME_MSECS))));
    } else {
        audioStreamStats->push_back(
                                    QString("Ringbuffer frames | desired: %1, avg_available(10s): %2, available: %3").arg(QString::number(streamStats->_desiredJitterBufferFrames)).arg(QString::number(streamStats->_framesAvailableAverage)).arg(QString::number(streamStats->_framesAvailable)));
    }
    
    audioStreamStats->push_back(
                                QString("Ringbuffer stats | starves: %1, prev_starve_lasted: %2, frames_dropped: %3, overflows: %4").arg(QString::number(streamStats->_starveCount)).arg(QString::number(streamStats->_consecutiveNotMixedCount)).arg(QString::number(streamStats->_framesDropped)).arg(QString::number(streamStats->_overflowCount)));
    audioStreamStats->push_back(
                                QString("Inter-packet timegaps (overall) | min: %1, max: %2, avg: %3").arg(formatUsecTime(streamStats->_timeGapMin).toLatin1().data()).arg(formatUsecTime(streamStats->_timeGapMax).toLatin1().data()).arg(formatUsecTime(streamStats->_timeGapAverage).toLatin1().data()));
    audioStreamStats->push_back(
                                QString("Inter-packet timegaps (last 30s) | min: %1, max: %2, avg: %3").arg(formatUsecTime(streamStats->_timeGapWindowMin).toLatin1().data()).arg(formatUsecTime(streamStats->_timeGapWindowMax).toLatin1().data()).arg(QString::number(streamStats->_timeGapWindowAverage).toLatin1().data()));
    
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
    setFixedSize(width(), height());
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


