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
#include <NodeData.h>

class AvatarMixerClientData : public NodeData {
    Q_OBJECT
public:
    AvatarMixerClientData();

    int parseData(const QByteArray& packet);
    
    bool hasSentIdentityBetweenKeyFrames() const { return _hasSentIdentityBetweenKeyFrames; }
    void setHasSentIdentityBetweenKeyFrames(bool hasSentIdentityBetweenKeyFrames)
        { _hasSentIdentityBetweenKeyFrames = hasSentIdentityBetweenKeyFrames; }
    
    bool hasSentBillboardBetweenKeyFrames() const { return _hasSentBillboardBetweenKeyFrames; }
    void setHasSentBillboardBetweenKeyFrames(bool hasSentBillboardBetweenKeyFrames)
        { _hasSentBillboardBetweenKeyFrames = hasSentBillboardBetweenKeyFrames; }

    AvatarData& getAvatar() { return _avatar; }
        
private:
   
    bool _hasSentIdentityBetweenKeyFrames;
    bool _hasSentBillboardBetweenKeyFrames;
    AvatarData _avatar;
};

#endif /* defined(__hifi__AvatarMixerClientData__) */
