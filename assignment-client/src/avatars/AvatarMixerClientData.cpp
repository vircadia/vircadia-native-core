//
//  AvatarMixerClientData.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "AvatarMixerClientData.h"

void AvatarMixerClientData::parseIdentityPacket(const QByteArray &packet) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    QUrl faceModelURL, skeletonURL;
    packetStream >> faceModelURL >> skeletonURL;
    
    if (faceModelURL != _faceModelURL) {
        _faceModelURL = faceModelURL;
    }
    
    if (skeletonURL != _skeletonURL) {
        _skeletonURL = skeletonURL;
    }
}