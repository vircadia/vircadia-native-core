//
//  AvatarMixerClientData.h
//  hifi
//
//  Created by Stephen Birarda on 2/4/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarMixerClientData__
#define __hifi__AvatarMixerClientData__

#include <QtCore/QMutex>
#include <QtCore/QUrl>

#include <AvatarData.h>
#include <NodeData.h>

class AvatarMixerClientData : public NodeData {
    Q_OBJECT
public:
    AvatarMixerClientData();

    int parseData(const QByteArray& packet);
    AvatarData& getAvatar() { return _avatar; }
    
    bool checkAndSetHasReceivedFirstPackets();
    
    quint64 getBillboardChangeTimestamp() const { return _billboardChangeTimestamp; }
    void setBillboardChangeTimestamp(quint64 billboardChangeTimestamp) { _billboardChangeTimestamp = billboardChangeTimestamp; }
    
    quint64 getIdentityChangeTimestamp() const { return _identityChangeTimestamp; }
    void setIdentityChangeTimestamp(quint64 identityChangeTimestamp) { _identityChangeTimestamp = identityChangeTimestamp; }
    
private:
    AvatarData _avatar;
    bool _hasReceivedFirstPackets;
    quint64 _billboardChangeTimestamp;
    quint64 _identityChangeTimestamp;
};

#endif /* defined(__hifi__AvatarMixerClientData__) */
