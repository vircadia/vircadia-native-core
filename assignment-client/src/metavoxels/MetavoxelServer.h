//
//  MetavoxelServer.h
//  hifi
//
//  Created by Andrzej Kapolka on 12/28/2013.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__MetavoxelServer__
#define __hifi__MetavoxelServer__

#include <ThreadedAssignment.h>

#include <MetavoxelData.h>

/// Maintains a shared metavoxel system, accepting change requests and broadcasting updates.
class MetavoxelServer : public ThreadedAssignment {
    Q_OBJECT

public:
    
    MetavoxelServer(const unsigned char* dataBuffer, int numBytes);
    
    virtual void run();
    
    virtual void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);

private:
    
    MetavoxelData _data;
};

#endif /* defined(__hifi__MetavoxelServer__) */
