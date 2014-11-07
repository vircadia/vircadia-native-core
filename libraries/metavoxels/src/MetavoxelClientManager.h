//
//  MetavoxelClientManager.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 6/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelClientManager_h
#define hifi_MetavoxelClientManager_h

#include <QReadWriteLock>
#include <QTimer>

#include "Endpoint.h"

class MetavoxelClient;
class MetavoxelEditMessage;
class MetavoxelUpdater;

/// Manages the set of connected metavoxel clients.
class MetavoxelClientManager : public QObject {
    Q_OBJECT

public:

    MetavoxelClientManager();
    virtual ~MetavoxelClientManager();

    virtual void init();

    MetavoxelUpdater* getUpdater() const { return _updater; }

    SharedObjectPointer findFirstRaySpannerIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const AttributePointer& attribute, float& distance);

    Q_INVOKABLE void paintHeightfieldHeight(const glm::vec3& position, float radius, float height);

    Q_INVOKABLE void applyEdit(const MetavoxelEditMessage& edit, bool reliable = false);

    /// Returns the current LOD.  This must be thread-safe, as it will be called from the updater thread.
    virtual MetavoxelLOD getLOD();
    
private slots:

    void maybeAttachClient(const SharedNodePointer& node);
    void maybeDeleteClient(const SharedNodePointer& node);
    
protected:
    
    virtual MetavoxelClient* createClient(const SharedNodePointer& node);
    
    void guide(MetavoxelVisitor& visitor);
    
    MetavoxelUpdater* _updater;
};

/// Handles updates in a dedicated thread.
class MetavoxelUpdater : public QObject {
    Q_OBJECT

public:
    
    MetavoxelUpdater(MetavoxelClientManager* clientManager);
    
    const MetavoxelLOD& getLOD() const { return _lod; }
    
    Q_INVOKABLE void start();
    
    Q_INVOKABLE void addClient(QObject* client);
    
    Q_INVOKABLE void applyEdit(const MetavoxelEditMessage& edit, bool reliable);
    
    /// Requests a set of statistics.  The receiving method should take six integer arguments: internal node count, leaf count,
    /// send progress, send total, receive progress, receive total.
    Q_INVOKABLE void getStats(QObject* receiver, const QByteArray& method);
    
private slots:
    
    void sendUpdates();    
    void removeClient(QObject* client);
    
private:
    
    MetavoxelClientManager* _clientManager;
    QSet<MetavoxelClient*> _clients;
    
    QTimer _sendTimer;
    qint64 _lastSend;
    
    MetavoxelLOD _lod;
};

/// Base class for metavoxel clients.
class MetavoxelClient : public Endpoint {
    Q_OBJECT

public:
    
    MetavoxelClient(const SharedNodePointer& node, MetavoxelUpdater* updater);

    /// Returns a reference to the most recent data.  This function is *not* thread-safe.
    const MetavoxelData& getData() const { return _data; }

    /// Returns a copy of the most recent data.  This function *is* thread-safe.
    MetavoxelData getDataCopy();

    void applyEdit(const MetavoxelEditMessage& edit, bool reliable = false);

protected:

    PacketRecord* getAcknowledgedSendRecord(int packetNumber) const;
    PacketRecord* getAcknowledgedReceiveRecord(int packetNumber) const;

    virtual void dataChanged(const MetavoxelData& oldData);

    virtual void recordReceive();
    
    virtual void clearSendRecordsBefore(int index);
    virtual void clearReceiveRecordsBefore(int index);
    
    virtual void writeUpdateMessage(Bitstream& out);
    virtual void handleMessage(const QVariant& message, Bitstream& in);

    virtual PacketRecord* maybeCreateSendRecord() const;
    virtual PacketRecord* maybeCreateReceiveRecord() const;

    MetavoxelUpdater* _updater;
    MetavoxelData _data;
    MetavoxelData _remoteData;
    MetavoxelLOD _remoteDataLOD;
    
    ReliableChannel* _reliableDeltaChannel;
    MetavoxelLOD _reliableDeltaLOD;
    int _reliableDeltaID;
    QVariant _reliableDeltaMessage;
    
    MetavoxelData _dataCopy;
    QReadWriteLock _dataCopyLock;
    
    QDataStream _dummyDataStream;
    Bitstream _dummyInputStream;
    int _dummyPacketNumber;
    QList<PacketRecord*> _clearedSendRecords;
    
    typedef QPair<PacketRecord*, Bitstream::ReadMappings> ClearedReceiveRecord;
    QList<ClearedReceiveRecord> _clearedReceiveRecords;
};

#endif // hifi_MetavoxelClientManager_h
