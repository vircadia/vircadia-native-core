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

#include <queue>

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

    void queuePacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer node);
    void processPackets();

    // locks the mutex to make a copy
    AudioStreamMap getAudioStreams() { QReadLocker readLock { &_streamsLock }; return _audioStreams; }
    AvatarAudioStream* getAvatarAudioStream();

    // returns whether self (this data's node) should ignore node, memoized by frame
    // precondition: frame is increasing after first call (including overflow wrap)
    bool shouldIgnore(SharedNodePointer self, SharedNodePointer node, unsigned int frame);

    // the following methods should be called from the AudioMixer assignment thread ONLY
    // they are not thread-safe

    // returns a new or existing HRTF object for the given stream from the given node
    AudioHRTF& hrtfForStream(const QUuid& nodeID, const QUuid& streamID = QUuid()) { return _nodeSourcesHRTFMap[nodeID][streamID]; }

    // removes an AudioHRTF object for a given stream
    void removeHRTFForStream(const QUuid& nodeID, const QUuid& streamID = QUuid());

    // remove all sources and data from this node
    void removeNode(const QUuid& nodeID) { _nodeSourcesIgnoreMap.unsafe_erase(nodeID); _nodeSourcesHRTFMap.erase(nodeID); }

    void removeAgentAvatarAudioStream();

    // packet parsers
    int parseData(ReceivedMessage& message) override;
    void negotiateAudioFormat(ReceivedMessage& message, const SharedNodePointer& node);
    void parseRequestsDomainListData(ReceivedMessage& message);
    void parsePerAvatarGainSet(ReceivedMessage& message, const SharedNodePointer& node);
    void parseNodeIgnoreRequest(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& node);
    void parseRadiusIgnoreRequest(QSharedPointer<ReceivedMessage> message, const SharedNodePointer& node);

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
    bool getRequestsDomainListData() { return _requestsDomainListData; }
    void setRequestsDomainListData(bool requesting) { _requestsDomainListData = requesting; }

    void setupCodecForReplicatedAgent(QSharedPointer<ReceivedMessage> message);

signals:
    void injectorStreamFinished(const QUuid& streamIdentifier);

public slots:
    void handleMismatchAudioFormat(SharedNodePointer node, const QString& currentCodec, const QString& recievedCodec);
    void sendSelectAudioFormat(SharedNodePointer node, const QString& selectedCodecName);

private:
    struct PacketQueue : public std::queue<QSharedPointer<ReceivedMessage>> {
        QWeakPointer<Node> node;
    };
    PacketQueue _packetQueue;

    QReadWriteLock _streamsLock;
    AudioStreamMap _audioStreams; // microphone stream from avatar is stored under key of null UUID

    void optionallyReplicatePacket(ReceivedMessage& packet, const Node& node);

    using IgnoreZone = AABox;
    class IgnoreZoneMemo {
    public:
        IgnoreZoneMemo(AudioMixerClientData& data) : _data(data) {}

        // returns an ignore zone, memoized by frame (lockless if the zone is already memoized)
        // preconditions:
        //  - frame is increasing after first call (including overflow wrap)
        //  - there are no references left from calls to getIgnoreZone(frame - 1)
        IgnoreZone& get(unsigned int frame);

    private:
        AudioMixerClientData& _data;
        IgnoreZone _zone;
        std::atomic<unsigned int> _frame { 0 };
        std::mutex _mutex;
    };
    IgnoreZoneMemo _ignoreZone;

    class IgnoreNodeCache {
    public:
        // std::atomic is not copyable - always initialize uncached
        IgnoreNodeCache() {}
        IgnoreNodeCache(const IgnoreNodeCache& other) {}

        void cache(bool shouldIgnore);
        bool isCached();
        bool shouldIgnore();

    private:
        std::atomic<bool> _isCached { false };
        bool _shouldIgnore { false };
    };
    struct IgnoreNodeCacheHasher { std::size_t operator()(const QUuid& key) const { return qHash(key); } };

    using NodeSourcesIgnoreMap = tbb::concurrent_unordered_map<QUuid, IgnoreNodeCache, IgnoreNodeCacheHasher>;
    NodeSourcesIgnoreMap _nodeSourcesIgnoreMap;

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
    bool _requestsDomainListData { false };
};

#endif // hifi_AudioMixerClientData_h
