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

#include <plugins/Forward.h>
#include <plugins/CodecPlugin.h>

#include "PositionalAudioStream.h"
#include "AvatarAudioStream.h"

class AudioMixerClientData : public NodeData {
    Q_OBJECT
public:
    AudioMixerClientData(const QUuid& nodeID, Node::LocalID nodeLocalID);
    ~AudioMixerClientData();

    using SharedStreamPointer = std::shared_ptr<PositionalAudioStream>;
    using AudioStreamVector = std::vector<SharedStreamPointer>;

    void queuePacket(QSharedPointer<ReceivedMessage> packet, SharedNodePointer node);
    void processPackets();

    AudioStreamVector& getAudioStreams() { return _audioStreams; }
    AvatarAudioStream* getAvatarAudioStream();

    // returns whether self (this data's node) should ignore node, memoized by frame
    // precondition: frame is increasing after first call (including overflow wrap)
    bool shouldIgnore(SharedNodePointer self, SharedNodePointer node, unsigned int frame);

    // the following methods should be called from the AudioMixer assignment thread ONLY
    // they are not thread-safe

    // returns a new or existing HRTF object for the given stream from the given node
    AudioHRTF& hrtfForStream(Node::LocalID nodeID, const QUuid& streamID = QUuid());

    // removes an AudioHRTF object for a given stream
    void removeHRTFForStream(Node::LocalID nodeID, const QUuid& streamID = QUuid());

    // remove all sources and data from this node
    void removeNode(Node::LocalID nodeID) { _nodeSourcesHRTFMap.erase(nodeID); }

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

    void setNodeLocalID(Node::LocalID localNodeID) { _localNodeID = localNodeID; }
    Node::LocalID getNodeLocalID() { return _localNodeID; }

    void sendAudioStreamStatsPackets(const SharedNodePointer& destinationNode);

    void incrementOutgoingMixedAudioSequenceNumber() { _outgoingMixedAudioSequenceNumber++; }
    quint16 getOutgoingSequenceNumber() const { return _outgoingMixedAudioSequenceNumber; }

    // uses randomization to have the AudioMixer send a stats packet to this node around every second
    bool shouldSendStats(int frameNumber);

    float getMasterAvatarGain() const { return _masterAvatarGain; }
    void setMasterAvatarGain(float gain) { _masterAvatarGain = gain; }

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
    AudioStreamVector _audioStreams; // microphone stream from avatar is stored under key of null UUID

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

    struct IdentifiedHRTF {
        QUuid streamIdentifier;
        std::unique_ptr<AudioHRTF> hrtf;
    };

    using HRTFVector = std::vector<IdentifiedHRTF>;
    using NodeSourcesHRTFMap = std::unordered_map<Node::LocalID, HRTFVector>;
    NodeSourcesHRTFMap _nodeSourcesHRTFMap;

    quint16 _outgoingMixedAudioSequenceNumber;

    AudioStreamStats _downstreamAudioStreamStats;

    int _frameToSendStats { 0 };

    float _masterAvatarGain { 1.0f };   // per-listener mixing gain, applied only to avatars

    CodecPluginPointer _codec;
    QString _selectedCodecName;
    Encoder* _encoder{ nullptr }; // for outbound mixed stream
    Decoder* _decoder{ nullptr }; // for mic stream

    bool _shouldFlushEncoder { false };

    bool _shouldMuteClient { false };
    bool _requestsDomainListData { false };

    Node::LocalID _localNodeID;
};

#endif // hifi_AudioMixerClientData_h
