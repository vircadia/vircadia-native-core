//
//  AvatarAudioStream.h
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarAudioStream_h
#define hifi_AvatarAudioStream_h

#include <QtCore/QUuid>

#include "PositionalAudioStream.h"

class AvatarAudioStream : public PositionalAudioStream {
public:
    AvatarAudioStream(bool isStereo, bool dynamicJitterBuffer, int staticDesiredJitterBufferFrames, int maxFramesOverDesired);

private:
    // disallow copying of AvatarAudioStream objects
    AvatarAudioStream(const AvatarAudioStream&);
    AvatarAudioStream& operator= (const AvatarAudioStream&);

    int parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples);
    int parseAudioData(PacketType type, const QByteArray& packetAfterStreamProperties, int numAudioSamples);
};

#endif // hifi_AvatarAudioStream_h
