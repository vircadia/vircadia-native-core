//
//  AvatarMixer.h
//  assignment-client/src/avatars
//
//  Created by Stephen Birarda on 9/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AvatarMixer__
#define __hifi__AvatarMixer__

#include <ThreadedAssignment.h>

/// Handles assignments of type AvatarMixer - distribution of avatar data to various clients
class AvatarMixer : public ThreadedAssignment {
public:
    AvatarMixer(const QByteArray& packet);
    ~AvatarMixer();
public slots:
    /// runs the avatar mixer
    void run();

    void nodeAdded(SharedNodePointer nodeAdded);
    void nodeKilled(SharedNodePointer killedNode);
    
    void readPendingDatagrams();
    
    void sendStatsPacket();
    
private:
    void broadcastAvatarData();
    
    QThread _broadcastThread;
    
    quint64 _lastFrameTimestamp;
    
    float _trailingSleepRatio;
    float _performanceThrottlingRatio;
    
    int _sumListeners;
    int _numStatFrames;
    int _sumBillboardPackets;
    int _sumIdentityPackets;
};

#endif /* defined(__hifi__AvatarMixer__) */
