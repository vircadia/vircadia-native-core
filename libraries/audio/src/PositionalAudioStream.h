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

#ifndef hifi_PositionalAudioRingBuffer_h
#define hifi_PositionalAudioRingBuffer_h

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

    PositionalAudioStream(PositionalAudioStream::Type type, bool isStereo = false, bool dynamicJitterBuffers = false);
    
    int parseData(const QByteArray& packet);
    
    virtual AudioStreamStats getAudioStreamStats() const;

    void updateNextOutputTrailingLoudness();
    float getNextOutputTrailingLoudness() const { return _nextOutputTrailingLoudness; }

    bool shouldLoopbackForNode() const { return _shouldLoopbackForNode; }
    bool isStereo() const { return _isStereo; }
    PositionalAudioStream::Type getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    AABox* getListenerUnattenuatedZone() const { return _listenerUnattenuatedZone; }

    void setListenerUnattenuatedZone(AABox* listenerUnattenuatedZone) { _listenerUnattenuatedZone = listenerUnattenuatedZone; }

protected:
    // disallow copying of PositionalAudioStream objects
    PositionalAudioStream(const PositionalAudioStream&);
    PositionalAudioStream& operator= (const PositionalAudioStream&);

    /// parses the info between the seq num and the audio data in the network packet and calculates
    /// how many audio samples this packet contains
    virtual int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) = 0;

    /// parses the audio data in the network packet
    virtual int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples) = 0;

    int parsePositionalData(const QByteArray& positionalByteArray);

protected:
    Type _type;
    glm::vec3 _position;
    glm::quat _orientation;

    bool _shouldLoopbackForNode;
    bool _isStereo;

    float _nextOutputTrailingLoudness;
    AABox* _listenerUnattenuatedZone;
};

#endif // hifi_PositionalAudioRingBuffer_h
