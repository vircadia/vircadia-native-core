//
//  AvatarMixer.h
//  hifi
//
//  Created by Stephen Birarda on 9/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarMixer__
#define __hifi__AvatarMixer__

#include <ThreadedAssignment.h>

/// Handles assignments of type AvatarMixer - distribution of avatar data to various clients
class AvatarMixer : public ThreadedAssignment {
public:
    AvatarMixer(const QByteArray& packet);
    
public slots:
    /// runs the avatar mixer
    void run();

    void nodeAdded(SharedNodePointer nodeAdded);
    void nodeKilled(SharedNodePointer killedNode);
    
    void readPendingDatagrams();
    
    void sendStatsPacket();
    
private:
    void broadcastAvatarData();
    
    int _sumListeners;
    int _numStatFrames;
};

#endif /* defined(__hifi__AvatarMixer__) */
