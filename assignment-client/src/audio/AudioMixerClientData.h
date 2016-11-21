//
//  AudioMixerClientData.h
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixerClientData_h
#define hifi_AudioMixerClientData_h

#include <QtCore/QJsonObject>

#include <AABox.h>
#include <AudioHRTF.h>
#include <AudioLimiter.h>
#include <UUIDHasher.h>

#include <plugins/CodecPlugin.h>

#include "PositionalAudioStream.h"
#include "AvatarAudioStream.h"


class AudioMixerClientData : public NodeData {
    Q_OBJECT
public:
    AudioMixerClientData(const QUuid& nodeID);
    ~AudioMixerClientData();

    using SharedStreamPointer = std::shared_ptr<PositionalAudioStream>;
    using AudioStreamMap = std::unordered_map<QUuid, SharedStreamPointer>;

    // locks the mutex to make a copy
    AudioStreamMap getAudioStreams() { QReadLocker readLock { &_streamsLock }; return _audioStreams; }
    AvatarAudioStream* getAvatarAudioStream();

    // the following methods should be called from the AudioMixer assignment thread ONLY
    // they are not thread-safe

    // returns a new or existing HRTF object for the given stream from the given node
    AudioHRTF& hrtfForStream(const QUuid& nodeID, const QUuid& streamID = QUuid()) { return _nodeSourcesHRTFMap[nodeID][streamID]; }

    // remove HRTFs for all sources from this node
    void removeHRTFsForNode(const QUuid& nodeID) { _nodeSourcesHRTFMap.erase(nodeID); }

    // removes an AudioHRTF object for a given stream
    void removeHRTFForStream(const QUuid& nodeID, const QUuid& streamID = QUuid());

    void removeAgentAvatarAudioStream();

    int parseData(ReceivedMessage& message) override;

    // attempt to pop a frame from each audio stream, and return the number of streams from this client
    int checkBuffersBeforeFrameSend();

    void removeDeadInjectedStreams();

    QJsonObject getAudioStreamStats();

    void sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode);

    void incrementOutgoingMixedAudioSequenceNumber() { _outgoingMixedAudioSequenceNumber++; }
    quint16 getOutgoingSequenceNumber() const { return _outgoingMixedAudioSequenceNumber; }

    // uses randomization to have the AudioMixer send a stats packet to this node around every second
    bool shouldSendStats(int frameNumber);

    AudioLimiter audioLimiter;

    void setupCodec(CodecPluginPointer codec, const QString& codecName);
    void cleanupCodec();
    void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) {
        if (_encoder) {
            _encoder->encode(decodedBuffer, encodedBuffer);
        } else {
            encodedBuffer = decodedBuffer;
        }
        // once you have encoded, you need to flush eventually.
        _shouldFlushEncoder = true;
    }
    void encodeFrameOfZeros(QByteArray& encodedZeros);
    bool shouldFlushEncoder() { return _shouldFlushEncoder; }

    QString getCodecName() { return _selectedCodecName; }
    
    bool shouldMuteClient() { return _shouldMuteClient; }
    void setShouldMuteClient(bool shouldMuteClient) { _shouldMuteClient = shouldMuteClient; }
    glm::vec3 getPosition() { return getAvatarAudioStream() ? getAvatarAudioStream()->getPosition() : glm::vec3(0); }

signals:
    void injectorStreamFinished(const QUuid& streamIdentifier);

public slots:
    void handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec);
    void sendSelectAudioFormat(SharedNodePointer node, const QString& selectedCodecName);

private:
    QReadWriteLock _streamsLock;
    AudioStreamMap _audioStreams; // microphone stream from avatar is stored under key of null UUID

    using HRTFMap = std::unordered_map<QUuid, AudioHRTF>;
    using NodeSourcesHRTFMap = std::unordered_map<QUuid, HRTFMap>;
    NodeSourcesHRTFMap _nodeSourcesHRTFMap;

    quint16 _outgoingMixedAudioSequenceNumber;

    AudioStreamStats _downstreamAudioStreamStats;

    int _frameToSendStats { 0 };

    CodecPluginPointer _codec;
    QString _selectedCodecName;
    Encoder* _encoder{ nullptr }; // for outbound mixed stream
    Decoder* _decoder{ nullptr }; // for mic stream

    bool _shouldFlushEncoder { false };

    bool _shouldMuteClient { false };
};

#endif // hifi_AudioMixerClientData_h
