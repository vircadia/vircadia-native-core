//
//  AvatarMixerClientData.h
//  hifi
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarMixerClientData__
#define __hifi__AvatarMixerClientData__

#include <QtCore/QUrl>

#include <AvatarData.h>

class AvatarMixerClientData : public AvatarData {
public:
    AvatarMixerClientData();
    
    const QUrl& getFaceModelURL() const { return _faceModelURL; }
    void setFaceModelURL(const QUrl& faceModelURL) { _faceModelURL = faceModelURL; }
    
    const QUrl& getSkeletonURL() const { return _skeletonURL; }
    void setSkeletonURL(const QUrl& skeletonURL) { _skeletonURL = skeletonURL; }
    
    void setHasSentPacketBetweenKeyFrames(bool hasSentPacketBetweenKeyFrames)
        { _hasSentPacketBetweenKeyFrames = hasSentPacketBetweenKeyFrames; }
    
    bool shouldSendIdentityPacketAfterParsing(const QByteArray& packet);
private:
    QUrl _faceModelURL;
    QUrl _skeletonURL;
    bool _hasSentPacketBetweenKeyFrames;
};

#endif /* defined(__hifi__AvatarMixerClientData__) */
