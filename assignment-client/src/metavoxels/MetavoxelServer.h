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

class MetavoxelSession;

/// Maintains a shared metavoxel system, accepting change requests and broadcasting updates.
class MetavoxelServer : public ThreadedAssignment {
    Q_OBJECT

public:
    
    MetavoxelServer(const QByteArray& packet);

    const MetavoxelDataPointer& getData() const { return _data; }

    void removeSession(const QUuid& sessionId);

    virtual void run();
    
    virtual void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);

private slots:
    
    void sendDeltas();    
    
private:
    
    void processData(const QByteArray& data, const HifiSockAddr& sender);
    
    QTimer _sendTimer;
    qint64 _lastSend;
    
    QHash<QUuid, MetavoxelSession*> _sessions;
    
    MetavoxelDataPointer _data;
};

/// Contains the state of a single client session.
class MetavoxelSession : public QObject {
    Q_OBJECT
    
public:
    
    MetavoxelSession(MetavoxelServer* server, const QUuid& sessionId, const QByteArray& datagramHeader);

    void receivedData(const QByteArray& data, const HifiSockAddr& sender);

    void sendDelta();

private slots:

    void timedOut();

    void sendData(const QByteArray& data);

    void readPacket(Bitstream& in);    
    
    void clearSendRecordsBefore(int index);
    
private:
    
    void handleMessage(const QVariant& message, Bitstream& in);
    
    class SendRecord {
    public:
        int packetNumber;
        MetavoxelDataPointer data;
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
