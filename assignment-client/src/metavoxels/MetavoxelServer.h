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
#include <QTimer>
#include <QUuid>

#include <HifiSockAddr.h>
#include <ThreadedAssignment.h>

#include <DatagramSequencer.h>
#include <MetavoxelData.h>

class MetavoxelSession;

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
    
    QHash<QUuid, MetavoxelSession*> _sessions;
};

/// Contains the state of a single client session.
class MetavoxelSession : public QObject {
    Q_OBJECT
    
public:
    
    MetavoxelSession(MetavoxelServer* server, const QUuid& sessionId, const QByteArray& datagramHeader);

    void receivedData(const QByteArray& data, const HifiSockAddr& sender);

private slots:

    void timedOut();

    void sendData(const QByteArray& data);

    void readPacket(Bitstream& in);    
    
private:
    
    MetavoxelServer* _server;
    QUuid _sessionId;
    
    QTimer _timeoutTimer;
    DatagramSequencer _sequencer;
    
    HifiSockAddr _sender;
};

#endif /* defined(__hifi__MetavoxelServer__) */
