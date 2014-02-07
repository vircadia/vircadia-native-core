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
#include <QList>
#include <QTimer>
#include <QUuid>

#include <HifiSockAddr.h>
#include <ThreadedAssignment.h>

#include <DatagramSequencer.h>
#include <MetavoxelData.h>

class MetavoxelEditMessage;
class MetavoxelSession;

/// Maintains a shared metavoxel system, accepting change requests and broadcasting updates.
class MetavoxelServer : public ThreadedAssignment {
    Q_OBJECT

public:
    
    MetavoxelServer(const QByteArray& packet);

    void applyEdit(const MetavoxelEditMessage& edit);

    const MetavoxelData& getData() const { return _data; }

    void removeSession(const QUuid& sessionId);

    virtual void run();
    
    virtual void readPendingDatagrams();

private slots:
    
    void sendDeltas();    
    
private:
    
    void processData(const QByteArray& data, const HifiSockAddr& sender);
    
    QTimer _sendTimer;
    qint64 _lastSend;
    
    QHash<QUuid, MetavoxelSession*> _sessions;
    
    MetavoxelData _data;
};

/// Contains the state of a single client session.
class MetavoxelSession : public QObject {
    Q_OBJECT
    
public:
    
    MetavoxelSession(MetavoxelServer* server, const QUuid& sessionId,
        const QByteArray& datagramHeader, const HifiSockAddr& sender);

    void receivedData(const QByteArray& data, const HifiSockAddr& sender);

    void sendDelta();

private slots:

    void timedOut();

    void sendData(const QByteArray& data);

    void readPacket(Bitstream& in);    
    
    void clearSendRecordsBefore(int index);
    
    void handleMessage(const QVariant& message);
    
private:
    
    class SendRecord {
    public:
        int packetNumber;
        MetavoxelData data;
    };
    
    MetavoxelServer* _server;
    QUuid _sessionId;
    
    QTimer _timeoutTimer;
    DatagramSequencer _sequencer;
    
    HifiSockAddr _sender;
    
    glm::vec3 _position;
    
    QList<SendRecord> _sendRecords;
};

#endif /* defined(__hifi__MetavoxelServer__) */
