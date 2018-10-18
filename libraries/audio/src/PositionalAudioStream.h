//
//  PositionalAudioStream.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PositionalAudioStream_h
#define hifi_PositionalAudioStream_h

#include <glm/gtx/quaternion.hpp>
#include <AABox.h>

#include "InboundAudioStream.h"

const int AUDIOMIXER_INBOUND_RING_BUFFER_FRAME_CAPACITY = 100;

using StreamID = QUuid;
const int NUM_STREAM_ID_BYTES = NUM_BYTES_RFC4122_UUID;

struct NodeIDStreamID {
    QUuid nodeID;
    Node::LocalID nodeLocalID;
    StreamID streamID;

    NodeIDStreamID(QUuid nodeID, Node::LocalID nodeLocalID, StreamID streamID)
        : nodeID(nodeID), nodeLocalID(nodeLocalID), streamID(streamID) {};

    bool operator==(const NodeIDStreamID& other) const {
        return (nodeLocalID == other.nodeLocalID || nodeID == other.nodeID) && streamID == other.streamID;
    }
};

using ChannelFlag = quint8;

class PositionalAudioStream : public InboundAudioStream {
    Q_OBJECT
public:
    enum Type {
        Microphone,
        Injector
    };

    PositionalAudioStream(PositionalAudioStream::Type type, bool isStereo, int numStaticJitterFrames = -1);

    const QUuid DEFAULT_STREAM_IDENTIFIER = QUuid();
    virtual const StreamID& getStreamIdentifier() const { return DEFAULT_STREAM_IDENTIFIER; }

    virtual void resetStats() override;

    virtual AudioStreamStats getAudioStreamStats() const override;

    void updateLastPopOutputLoudnessAndTrailingLoudness();
    float getLastPopOutputTrailingLoudness() const { return _lastPopOutputTrailingLoudness; }
    float getLastPopOutputLoudness() const { return _lastPopOutputLoudness; }
    float getQuietestFrameLoudness() const { return _quietestFrameLoudness; }

    bool shouldLoopbackForNode() const { return _shouldLoopbackForNode; }
    bool isStereo() const { return _isStereo; }

    PositionalAudioStream::Type getType() const { return _type; }

    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    const glm::vec3& getAvatarBoundingBoxCorner() const { return _avatarBoundingBoxCorner; }
    const glm::vec3& getAvatarBoundingBoxScale() const { return _avatarBoundingBoxScale; }

    using IgnoreBox = AABox;

    // called from single AudioMixerSlave while processing packets for node
    void enableIgnoreBox();
    void disableIgnoreBox() { _isIgnoreBoxEnabled = false; }

    // thread-safe, called from AudioMixerSlave(s) while preparing mixes
    bool isIgnoreBoxEnabled() const { return _isIgnoreBoxEnabled; }
    const IgnoreBox& getIgnoreBox() const { return _ignoreBox; }

protected:
    // disallow copying of PositionalAudioStream objects
    PositionalAudioStream(const PositionalAudioStream&);
    PositionalAudioStream& operator= (const PositionalAudioStream&);

    int parsePositionalData(const QByteArray& positionalByteArray);

protected:
    void calculateIgnoreBox();

    Type _type;
    glm::vec3 _position;
    glm::quat _orientation;

    glm::vec3 _avatarBoundingBoxCorner;
    glm::vec3 _avatarBoundingBoxScale;

    bool _shouldLoopbackForNode;
    bool _isStereo;
    // Ignore penumbra filter
    bool _ignorePenumbra;

    float _lastPopOutputTrailingLoudness;
    float _lastPopOutputLoudness;
    float _quietestTrailingFrameLoudness;
    float _quietestFrameLoudness;
    int _frameCounter;

    bool _isIgnoreBoxEnabled { false };
    IgnoreBox _ignoreBox;
};

#endif // hifi_PositionalAudioStream_h
