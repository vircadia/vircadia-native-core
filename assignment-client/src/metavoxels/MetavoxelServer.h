//
//  MetavoxelServer.h
//  hifi
//
//  Created by Andrzej Kapolka on 12/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__MetavoxelServer__
#define __hifi__MetavoxelServer__

#include <QHash>
#include <QUuid>

#include <ThreadedAssignment.h>

#include <MetavoxelData.h>

class Session;

/// Maintains a shared metavoxel system, accepting change requests and broadcasting updates.
class MetavoxelServer : public ThreadedAssignment {
    Q_OBJECT

public:
    
    MetavoxelServer(const unsigned char* dataBuffer, int numBytes);

    void removeSession(const QUuid& sessionId);

    virtual void run();
    
    virtual void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);

private:
    
    void processData(const QByteArray& data, const HifiSockAddr& sender);
    
    MetavoxelData _data;
    
    QHash<QUuid, Session*> _sessions;
};

#endif /* defined(__hifi__MetavoxelServer__) */
