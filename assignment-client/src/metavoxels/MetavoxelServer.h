//
//  MetavoxelServer.h
//  assignment-client/src/metavoxels
//
//  Created by Andrzej Kapolka on 12/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelServer_h
#define hifi_MetavoxelServer_h

#include <QList>
#include <QTimer>

#include <ThreadedAssignment.h>

#include <Endpoint.h>

class MetavoxelEditMessage;
class MetavoxelPersister;
class MetavoxelSender;
class MetavoxelSession;

/// Maintains a shared metavoxel system, accepting change requests and broadcasting updates.
class MetavoxelServer : public ThreadedAssignment {
    Q_OBJECT

public:
    
    MetavoxelServer(const QByteArray& packet);

    Q_INVOKABLE void applyEdit(const MetavoxelEditMessage& edit);

    const MetavoxelData& getData() const { return _data; }
    
    Q_INVOKABLE void setData(const MetavoxelData& data, bool loaded = false);

    virtual void run();
    
    virtual void readPendingDatagrams();
    
    virtual void aboutToFinish();

signals:

    void dataChanged(const MetavoxelData& data);

private slots:

    void maybeAttachSession(const SharedNodePointer& node);
    void maybeDeleteSession(const SharedNodePointer& node);   
    void maybeSaveData();
    
private:
    
    QVector<MetavoxelSender*> _senders;
    int _nextSender;
    
    MetavoxelPersister* _persister;
    
    MetavoxelData _data;
    MetavoxelData _savedData;
    bool _savedDataInitialized;
};

/// Handles update sending for one thread.
class MetavoxelSender : public QObject {
    Q_OBJECT

public:
    
    MetavoxelSender(MetavoxelServer* server);
    
    MetavoxelServer* getServer() const { return _server; }
    
    const MetavoxelData& getData() const { return _data; }
    
    Q_INVOKABLE void start();
    
    Q_INVOKABLE void addSession(QObject* session);
    
private slots:
    
    void setData(const MetavoxelData& data) { _data = data; }
    void sendDeltas();
    void removeSession(QObject* session);
    
private:
    
    MetavoxelServer* _server;
    QSet<MetavoxelSession*> _sessions;
    
    QTimer _sendTimer;
    qint64 _lastSend;
    
    MetavoxelData _data;
};

/// Contains the state of a single client session.
class MetavoxelSession : public Endpoint {
    Q_OBJECT
    
public:
    
    MetavoxelSession(const SharedNodePointer& node, MetavoxelSender* sender);
    
    virtual void update();

protected:

    virtual void handleMessage(const QVariant& message, Bitstream& in);
    
    virtual PacketRecord* maybeCreateSendRecord() const;

private slots:

    void handleMessage(const QVariant& message);
    void checkReliableDeltaReceived();
    
private:
    
    void sendPacketGroup(int alreadySent = 0);
    
    MetavoxelSender* _sender;
    
    MetavoxelLOD _lod;
    int _lodPacketNumber;
    
    ReliableChannel* _reliableDeltaChannel;
    int _reliableDeltaReceivedOffset;
    MetavoxelData _reliableDeltaData;
    MetavoxelLOD _reliableDeltaLOD;
    Bitstream::WriteMappings _reliableDeltaWriteMappings;
    int _reliableDeltaID;
    QVariant _reliableDeltaMessage;
};

/// Handles persistence in a separate thread.
class MetavoxelPersister : public QObject {
    Q_OBJECT

public:
    
    MetavoxelPersister(MetavoxelServer* server);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save(const MetavoxelData& data);

private:
    
    MetavoxelServer* _server;
};

#endif // hifi_MetavoxelServer_h
