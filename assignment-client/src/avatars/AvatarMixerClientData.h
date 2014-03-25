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
    AvatarData& getAvatar() { return _avatar; }
        
private:
    AvatarData _avatar;
};

#endif /* defined(__hifi__AvatarMixerClientData__) */
