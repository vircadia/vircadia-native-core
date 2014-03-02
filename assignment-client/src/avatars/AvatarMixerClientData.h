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
    Q_OBJECT
public:
    AvatarMixerClientData();
    
    bool hasSentIdentityBetweenKeyFrames() const { return _hasSentIdentityBetweenKeyFrames; }
    void setHasSentIdentityBetweenKeyFrames(bool hasSentIdentityBetweenKeyFrames)
        { _hasSentIdentityBetweenKeyFrames = hasSentIdentityBetweenKeyFrames; }
    
    bool hasSentBillboardBetweenKeyFrames() const { return _hasSentBillboardBetweenKeyFrames; }
    void setHasSentBillboardBetweenKeyFrames(bool hasSentBillboardBetweenKeyFrames)
        { _hasSentBillboardBetweenKeyFrames = hasSentBillboardBetweenKeyFrames; }
        
private:
   
    bool _hasSentIdentityBetweenKeyFrames;
    bool _hasSentBillboardBetweenKeyFrames;
};

#endif /* defined(__hifi__AvatarMixerClientData__) */
