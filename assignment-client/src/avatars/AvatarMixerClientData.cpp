//
//  AvatarMixerClientData.cpp
//  assignment-client/src/avatars
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <PacketHeaders.h>

#include "AvatarMixerClientData.h"

AvatarMixerClientData::AvatarMixerClientData() :
    NodeData(),
    _hasReceivedFirstPackets(false),
    _billboardChangeTimestamp(0),
    _identityChangeTimestamp(0)
{
    
}

int AvatarMixerClientData::parseData(const QByteArray& packet) {
    // compute the offset to the data payload
    int offset = numBytesForPacketHeader(packet);
    return _avatar.parseDataAtOffset(packet, offset);
}

bool AvatarMixerClientData::checkAndSetHasReceivedFirstPackets() {
    bool oldValue = _hasReceivedFirstPackets;
    _hasReceivedFirstPackets = true;
    return oldValue;
}
