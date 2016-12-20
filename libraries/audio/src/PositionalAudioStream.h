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

class PositionalAudioStream : public InboundAudioStream {
    Q_OBJECT
public:
    enum Type {
        Microphone,
        Injector
    };

    PositionalAudioStream(PositionalAudioStream::Type type, bool isStereo, int numStaticJitterFrames = -1);

    const QUuid DEFAULT_STREAM_IDENTIFIER = QUuid();
    virtual const QUuid& getStreamIdentifier() const { return DEFAULT_STREAM_IDENTIFIER; }

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


protected:
    // disallow copying of PositionalAudioStream objects
    PositionalAudioStream(const PositionalAudioStream&);
    PositionalAudioStream& operator= (const PositionalAudioStream&);

    int parsePositionalData(const QByteArray& positionalByteArray);

protected:
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
};

#endif // hifi_PositionalAudioStream_h
