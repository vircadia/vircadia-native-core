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

#include "Endpoint.h"

class MetavoxelClient;
class MetavoxelEditMessage;

/// Manages the set of connected metavoxel clients.
class MetavoxelClientManager : public QObject {
    Q_OBJECT

public:

    virtual void init();
    void update();

    SharedObjectPointer findFirstRaySpannerIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const AttributePointer& attribute, float& distance);

    Q_INVOKABLE void setSphere(const glm::vec3& center, float radius, const QColor& color = QColor(Qt::gray));

    Q_INVOKABLE void setSpanner(const SharedObjectPointer& object, bool reliable = false);
        
    Q_INVOKABLE void applyEdit(const MetavoxelEditMessage& edit, bool reliable = false);

    virtual MetavoxelLOD getLOD() const;
    
private slots:

    void maybeAttachClient(const SharedNodePointer& node);
    void maybeDeleteClient(const SharedNodePointer& node);
    
protected:
    
    virtual MetavoxelClient* createClient(const SharedNodePointer& node);
    virtual void updateClient(MetavoxelClient* client);
    
    void guide(MetavoxelVisitor& visitor);
};

/// Base class for metavoxel clients.
class MetavoxelClient : public Endpoint {
    Q_OBJECT

public:
    
    MetavoxelClient(const SharedNodePointer& node, MetavoxelClientManager* manager);

    MetavoxelData& getData() { return _data; }

    void guide(MetavoxelVisitor& visitor);
    
    void applyEdit(const MetavoxelEditMessage& edit, bool reliable = false);

protected:

    virtual void dataChanged(const MetavoxelData& oldData);

    virtual void writeUpdateMessage(Bitstream& out);
    virtual void handleMessage(const QVariant& message, Bitstream& in);

    virtual PacketRecord* maybeCreateSendRecord() const;
    virtual PacketRecord* maybeCreateReceiveRecord() const;

    MetavoxelClientManager* _manager;
    MetavoxelData _data;
    MetavoxelData _remoteData;
    MetavoxelLOD _remoteDataLOD;
    
    ReliableChannel* _reliableDeltaChannel;
    MetavoxelLOD _reliableDeltaLOD;
    int _reliableDeltaID;
};

#endif // hifi_MetavoxelClientManager_h
