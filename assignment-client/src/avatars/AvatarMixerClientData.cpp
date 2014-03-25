//
//  AvatarMixerClientData.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

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
