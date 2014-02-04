//
//  AvatarMixerClientData.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "AvatarMixerClientData.h"

AvatarMixerClientData::AvatarMixerClientData() :
    _faceModelURL(),
    _skeletonURL()
{
    
}

bool AvatarMixerClientData::shouldSendIdentityPacketAfterParsing(const QByteArray &packet) {
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    QUrl faceModelURL, skeletonURL;
    packetStream >> faceModelURL >> skeletonURL;
    
    bool hasIdentityChanged = false;
    
    if (faceModelURL != _faceModelURL) {
        _faceModelURL = faceModelURL;
        hasIdentityChanged = true;
    }
    
    if (skeletonURL != _skeletonURL) {
        _skeletonURL = skeletonURL;
        hasIdentityChanged = true;
    }
    
    return hasIdentityChanged && !_hasSentPacketBetweenKeyFrames;
}