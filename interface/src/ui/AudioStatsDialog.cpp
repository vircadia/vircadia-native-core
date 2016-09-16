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
    averageUpdateTimer->start(200);
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
    
    double mixerRingBufferFrames = 0.0,
        outputRingBufferFrames = 0.0;
    double audioInputBufferLatency = 0.0,
        inputRingBufferLatency = 0.0,
        networkRoundtripLatency = 0.0,
        mixerRingBufferLatency = 0.0,
        outputRingBufferLatency = 0.0,
        audioOutputBufferLatency = 0.0;
        
    if (SharedNodePointer audioMixerNodePointer = DependencyManager::get<NodeList>()->soloNodeOfType(NodeType::AudioMixer)) {
        audioInputBufferLatency = (double)_stats->getInputMsRead().getWindowMax();
        inputRingBufferLatency =  (double)_stats->getInputMsUnplayed().getWindowMax();
        networkRoundtripLatency = (double)audioMixerNodePointer->getPingMs();
        mixerRingBufferLatency = (double)_stats->getMixerAvatarStreamStats()._unplayedMs;
        outputRingBufferLatency = (double)_stats->getMixerDownstreamStats()._unplayedMs;
        audioOutputBufferLatency = (double)_stats->getOutputMsUnplayed().getWindowMax();
    }

    double totalLatency = audioInputBufferLatency + inputRingBufferLatency + mixerRingBufferLatency
        + outputRingBufferLatency + audioOutputBufferLatency + networkRoundtripLatency;

    QString stats;
    _audioMixerStats.push_back("PIPELINE (averaged over the past 10s)");
    stats = "Input Read:\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(audioInputBufferLatency, 'f', 0)));
    stats = "Input Ring:\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(inputRingBufferLatency, 'f', 0)));
    stats = "Network (client->mixer):\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(networkRoundtripLatency / 2, 'f', 0)));
    stats = "Mixer Ring:\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(mixerRingBufferLatency, 'f', 0)));
    stats = "Network (mixer->client):\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(networkRoundtripLatency / 2, 'f', 0)));
    stats = "Output Ring:\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(outputRingBufferLatency, 'f', 0)));
    stats = "Output Read:\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(audioOutputBufferLatency, 'f', 0)));
    stats = "TOTAL:\t%1 ms";
    _audioMixerStats.push_back(stats.arg(QString::number(totalLatency, 'f', 0)));

    const MovingMinMaxAvg<quint64>& packetSentTimeGaps = _stats->getPacketTimegaps();

    _upstreamClientStats.push_back("\nUpstream Mic Audio Packets Sent Gaps (by client):");

    stats = "Inter-packet timegaps";
    _upstreamClientStats.push_back(stats);
    stats = "overall min:\t%1, max:\t%2, avg:\t%3";
    stats = stats.arg(formatUsecTime(packetSentTimeGaps.getMin()),
                      formatUsecTime(packetSentTimeGaps.getMax()),
                      formatUsecTime(packetSentTimeGaps.getAverage()));
    _upstreamClientStats.push_back(stats);

    stats = "last window min:\t%1, max:\t%2, avg:\t%3";
    stats = stats.arg(formatUsecTime(packetSentTimeGaps.getWindowMin()),
                      formatUsecTime(packetSentTimeGaps.getWindowMax()),
                      formatUsecTime(packetSentTimeGaps.getWindowAverage()));
    _upstreamClientStats.push_back(stats);
    
    _upstreamMixerStats.push_back("\nMIXER STREAM");
    _upstreamMixerStats.push_back("(this client's remote mixer stream performance)");
        
    renderAudioStreamStats(&_stats->getMixerAvatarStreamStats(), &_upstreamMixerStats);
    
    _downstreamStats.push_back("\nCLIENT STREAM");
        
    AudioStreamStats downstreamStats = _stats->getMixerDownstreamStats();
    
    renderAudioStreamStats(&downstreamStats, &_downstreamStats);
   
    
    if (_shouldShowInjectedStreams) {

        foreach(const AudioStreamStats& injectedStreamAudioStats, _stats->getMixerInjectedStreamStatsMap()) {
            stats = "\nINJECTED STREAM (ID: %1)";
            stats = stats.arg(injectedStreamAudioStats._streamIdentifier.toString());
            _upstreamInjectedStats.push_back(stats);
            
            renderAudioStreamStats(&injectedStreamAudioStats, &_upstreamInjectedStats);
        }
 
    }
}


void AudioStatsDialog::renderAudioStreamStats(const AudioStreamStats* streamStats, QVector<QString>* audioStreamStats) {

    QString stats = "Packet Loss";
    audioStreamStats->push_back(stats);
    stats = "overall:\t%1%\t(%2 lost), window:\t%3%\t(%4 lost)";
    stats = stats.arg(QString::number((int)(streamStats->_packetStreamStats.getLostRate() * 100.0f)),
        QString::number((int)(streamStats->_packetStreamStats._lost)),
        QString::number((int)(streamStats->_packetStreamWindowStats.getLostRate() * 100.0f)),
        QString::number((int)(streamStats->_packetStreamWindowStats._lost)));
    audioStreamStats->push_back(stats);

    stats = "Ringbuffer";
    audioStreamStats->push_back(stats);
    stats = "available frames (avg):\t%1\t(%2), desired:\t%3";
    stats = stats.arg(QString::number(streamStats->_framesAvailable),
        QString::number(streamStats->_framesAvailableAverage),
        QString::number(streamStats->_desiredJitterBufferFrames));
    audioStreamStats->push_back(stats);
    stats = "starves:\t%1, last starve duration:\t%2, drops:\t%3, overflows:\t%4";
    stats = stats.arg(QString::number(streamStats->_starveCount),
        QString::number(streamStats->_consecutiveNotMixedCount),
        QString::number(streamStats->_framesDropped),
        QString::number(streamStats->_overflowCount));
    audioStreamStats->push_back(stats);

    stats = "Inter-packet timegaps";
    audioStreamStats->push_back(stats);

    stats = "overall min:\t%1, max:\t%2, avg:\t%3";
    stats = stats.arg(formatUsecTime(streamStats->_timeGapMin),
        formatUsecTime(streamStats->_timeGapMax),
        formatUsecTime(streamStats->_timeGapAverage));
    audioStreamStats->push_back(stats);


    stats = "last window min:\t%1, max:\t%2, avg:\t%3";
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


